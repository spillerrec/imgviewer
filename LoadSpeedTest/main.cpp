/*	This file is part of imgviewer, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */ 

#include <QCoreApplication>
#include <QFile> //For static rename
#include <QImage>
#include <QElapsedTimer>
#include <QTest>

#include <random>


class TimedIODevice : public QIODevice {
	private:
		QIODevice& dev;
		struct record{
			unsigned start;
			unsigned end;
			unsigned amount;
		};
		std::vector<record> records;
		QElapsedTimer timer;
	
	public:
		TimedIODevice( QIODevice& dev ) : dev(dev) {
			records.reserve( 1024 );
			timer.start();
		}
	
		bool atEnd() const override{ return dev.atEnd(); }
		qint64 bytesAvailable() const override{ return dev.bytesAvailable(); }
		qint64 bytesToWrite() const override{ return dev.bytesToWrite(); }
		bool canReadLine() const override{ return dev.canReadLine(); }
		void close() override{ dev.close(); }
		bool isSequential() const override{ return dev.isSequential(); }
	//	bool open( OpenMode mode ) override{ qDebug() << "calling open"; /*return dev.open(mode);*/ }
		bool reset() override{ return dev.reset(); }
		bool seek( qint64 pos ) override{ return dev.seek( pos ); }
		qint64 size() const{ return dev.size(); }
		bool waitForBytesWritten( int msec ){ return dev.waitForBytesWritten(msec); }
		bool waitForReadyRead( int msec ){ return dev.waitForReadyRead( msec ); }
		
		void print() const;
		
	protected:
		qint64 readData( char* data, qint64 maxSize );
		qint64 writeData( const char* data, qint64 maxSize );
};

qint64 TimedIODevice::readData( char* data, qint64 maxSize ){
	record r;
	r.amount = maxSize;
	
	//Time
	r.start = timer.elapsed();
	auto bytes = dev.read( data, maxSize );
	r.end = timer.elapsed();
	
	records.push_back( r );
	return bytes;
}

qint64 TimedIODevice::writeData( const char* data, qint64 maxSize ){
	record r;
	r.amount = maxSize;
	
	//Time
	r.start = timer.elapsed();
	auto bytes = dev.write( data, maxSize );
	r.end = timer.elapsed();
	
	records.push_back( r );
	return bytes;
}

void TimedIODevice::print() const{
	for( auto r : records ){
		qDebug() << r.start << ", " << r.end << ", " << r.amount;
	}
}


inline int printError( const char* const err, int error_code=-1 ){
	qDebug( err );
	return error_code;
}

int timeLoading( QString path ){
	QFile data( path );
	if( !data.open(QIODevice::ReadOnly) )
		return printError( "Could not find file" );
	
	QImage img;
	TimedIODevice timer(data);
	auto ext = QFileInfo(path).suffix().toLatin1();
	qDebug() << "Reading:" << ext.constData();
	img.load( &timer, ext.constData() );
	if( img.isNull() )
		return printError( "Image did not decode" );
	
	timer.print();
	
	return 0;
}

int main( int argc, char* argv[] ){
	QCoreApplication app( argc, argv );
	auto args = app.arguments();
	
	//Get 'from' and 'to' directory paths
	if( args.size() != 2 )
		return printError( "LoadSpeedTest IMAGE_PATH" );
	
	timeLoading( args[1] );
	
	QFile data( args[1] );
	if( !data.open(QIODevice::ReadOnly) )
		return printError( "Could not find file" );
	
	QElapsedTimer t;
	t.start();
	auto buffer = data.readAll();
	qDebug() << "Time for file loading:" << t.elapsed() << "ms";
	qDebug() << "File in bytes:" << buffer.size();
	
	auto ext = QFileInfo(args[1]).suffix().toLatin1();
	const int trials = 1;
	double total = 0;
	for( int i=0; i<trials; i++ ){
		t.start();
		auto img = QImage::fromData( buffer, ext.constData() );
		if( img.isNull() )
			return printError("Could not decode image");
		auto time = t.elapsed();
		total += time;
		qDebug() << "Trial" << i << " took " << time << "ms";
	}
	
	qDebug() << "Average: " << (total / trials) << "ms";
	
	return 0;
}
