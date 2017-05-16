/*	This file is part of imgviewer, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */ 

#include <QCoreApplication>
#include <QFile> //For static rename
#include <QImage>
#include <QElapsedTimer>
#include <QTest>

#include <random>


inline int printError( const char* const err, int error_code=-1 ){
	qDebug( err );
	return error_code;
}

int main( int argc, char* argv[] ){
	QCoreApplication app( argc, argv );
	auto args = app.arguments();
	
	//Get 'from' and 'to' directory paths
	if( args.size() != 2 )
		return printError( "LoadSpeedTest IMAGE_PATH" );
	
	QFile data( args[1] );
	if( !data.open(QIODevice::ReadOnly) )
		return printError( "Could not find file" );
	
	QElapsedTimer t;
	t.start();
	auto buffer = data.readAll();
	qDebug() << "Time for file loading:" << t.elapsed() << "ms";
	qDebug() << "File in bytes:" << buffer.size();
	
	auto ext = QFileInfo(args[1]).suffix().toLatin1();
	const int trials = 100;
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
