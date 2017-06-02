/*	This file is part of imgviewer, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */ 

#include <QCoreApplication>
#include <QFile> //For static rename
#include <QImage>
#include <QElapsedTimer>
#include <QTest>

#include <random>
#include <string>

static std::string getFormat( QString path ){
	auto ext = QFileInfo(path).suffix().toLower();
	
	return {ext.toLocal8Bit().constData()};
}


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
		void close() override{
			QIODevice::close();
			dev.close();
		}
		bool isSequential() const override{ return dev.isSequential(); }
		bool open( OpenMode mode ) override{
			QIODevice::open(mode);
			return dev.open(mode);
		}
		bool reset() override{
			QIODevice::reset();
			return dev.reset();
		}
		bool seek( qint64 pos ) override{
			QIODevice::seek(pos);
			return dev.seek( pos );
		}
		qint64 size() const{ return dev.size(); }
		bool waitForBytesWritten( int msec ){ return dev.waitForBytesWritten(msec); }
		bool waitForReadyRead( int msec ){ return dev.waitForReadyRead( msec ); }
		
		void print() const;
		
	protected:
		qint64 readData( char* data, qint64 maxSize );
		qint64 writeData( const char* data, qint64 maxSize );
};

qint64 TimedIODevice::readData( char* data, qint64 maxSize ){
	qDebug() << "reading" << maxSize;
	record r;
	r.amount = maxSize;
	
	//Time
	r.start = timer.elapsed();
	auto bytes = dev.read( data, maxSize );
	r.end = timer.elapsed();
	
	qDebug() << "Read:" << bytes;
	
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
	for( auto r : records )
		qDebug() << r.start << ", " << r.end << ", " << r.amount;
	
	auto total_time = records.back().end;
//	auto total_time = timer.elapsed();
	qDebug() << "Time for file loading:" << total_time << "ms";
	//qDebug() << "File in bytes:" << buffer.size();
}


inline int printError( const char* const err, int error_code=-1 ){
	qDebug( err );
	return error_code;
}

static int timeLoading( QString path ){
	QFile data( path );
	TimedIODevice timer(data);
	
	QImage img;
	img.load( &timer, getFormat(path).c_str() );
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
	
	auto ext = getFormat(args[1]);
	const int trials = 1;
	double total = 0;
	for( int i=0; i<trials; i++ ){
		t.start();
		auto img = QImage::fromData( buffer, ext.c_str() );
		if( img.isNull() )
			return printError("Could not decode image");
		auto time = t.elapsed();
		total += time;
		qDebug() << "Trial" << i << " took " << time << "ms";
		img.save("test.png");
	}
	
	qDebug() << "Average: " << (total / trials) << "ms";
	
	return 0;
}
