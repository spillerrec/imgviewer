/*	This file is part of imgviewer, which is free software and is licensed
 * under the terms of the GNU GPL v3.0. (see http://www.gnu.org/licenses/ ) */ 

#include <QCoreApplication>
#include <QFile> //For static rename
#include <QDir>
#include <QTest>

#include <random>

static bool dirIsEmpty( QDir dir ){
	auto allFiles = dir.entryInfoList( QDir::NoDotAndDotDot | QDir::AllEntries );
	return allFiles.count() == 0;
}

inline int printError( const char* const err, int error_code=-1 ){
	qDebug( err );
	return error_code;
}

int main( int argc, char* argv[] ){
	QCoreApplication app( argc, argv );
	auto args = app.arguments();
	
	//Get 'from' and 'to' directory paths
	if( args.size() < 4 )
		return printError( "FileFuzzer FROM_DIR TO_DIR MOVE_SPEED [NAME_FILTER]" );
	QDir from( args[1] );
	QDir to(   args[2] );
	int wait_time = args[3].toInt();
	
	if( !dirIsEmpty( to ) )
		return printError( "'To' directory must be empty" );
	
	//Set filter if supplied
	if( args.size() >= 5 ){
		//TODO: support supplying a list?
		QString filter = args[4];
		from.setNameFilters( QStringList() << filter );
	}
	
	//Make list of files which keeps track of its placement
	auto files = from.entryList( QDir::Files );
	struct File{
		QString name;
		bool moved;
		File( QString name ) : name(name), moved(false) { }
	};
	std::vector<File> moves;
	for( auto file : files )
		moves.emplace_back( file );
	
	//Code for swapping a file between the two directories
	auto fromDir = from.absolutePath() + "/";
	auto toDir =   to  .absolutePath() + "/";
	auto move = [&](File& file){
		auto a = fromDir + file.name;
		auto b = toDir   + file.name;
		if( file.moved )
			std::swap( a, b );
		
		if( QFile::rename( a, b ) )
			file.moved = !file.moved;
	};
	
	std::random_device rd;
	std::mt19937 gen( rd() );
	std::uniform_int_distribution<> dis( 0, moves.size()-1 );
	while( true ){ //TODO: Stop it somehow
		move( moves[dis( gen )] );
		QTest::qSleep( wait_time );
	}
	
	//Make sure the files end in its original position
	for( auto& file : moves )
		if( file.moved )
			move( file );
		else
			qWarning("Failed to move file!");
	
	return 0;
}
