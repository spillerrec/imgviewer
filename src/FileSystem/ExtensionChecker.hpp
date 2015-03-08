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

#ifndef EXTENSION_CHECKER_HPP
#define EXTENSION_CHECKER_HPP

#include <QString>
#include <QStringList>

#include <algorithm>
#include <vector>


class ExtensionChecker{
	private:
		class ExtensionGroup{
			private:
				int lenght;
				std::vector<QString> exts;
				
			public:
				ExtensionGroup( int lenght, std::vector<QString>&& exts ) : lenght(lenght), exts(exts)
					{ std::sort( this->exts.begin(), this->exts.end() ); }
				
				bool matches( const QString& str ) const;
		};
		std::vector<ExtensionGroup> groups;
		
	public:
		explicit ExtensionChecker( QStringList exts );
		
		bool matches( const QString& str ) const{
			for( auto& group : groups )
				if( group.matches( str ) )
					return true;
			return false;
		}
};


#endif