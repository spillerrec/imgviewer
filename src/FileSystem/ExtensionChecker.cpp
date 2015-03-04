/*
	This file is part of imgviewer.

	imgviewer is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	imgviewer is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with imgviewer.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "ExtensionChecker.hpp"

using namespace std;

bool ExtensionChecker::ExtensionGroup::matches( QString str ) const{
	//Must be able to contain the extension and start the extension with '.'
	if( str.size() < lenght + 1 || str[str.size() - lenght - 1] != '.' )
		return false;
	
	auto ext = str.right( lenght );
	return any_of( exts.begin(), exts.end(), [=]( QString s ){ return s == ext; } );
}

ExtensionChecker::ExtensionChecker( QStringList exts ){
	sort( exts.begin(), exts.end(), []( QString a, QString b ){ return a.size() < b.size(); } );
	
	int pos=0;
	while( pos<exts.count() ){
		int lenght = exts[pos].size();
		vector<QString> exts_group;
		
		for( ; pos<exts.count() && exts[pos].size() == lenght; pos++ )
			exts_group.push_back( exts[pos] );
		
		groups.emplace_back( lenght, std::move(exts_group) );
	}
}