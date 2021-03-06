/*
  This file is part of LOOS.

  LOOS (Lightweight Object-Oriented Structure library)
  Copyright (c) 2012, Tod D. Romo, Alan Grossfield
  Department of Biochemistry and Biophysics
  School of Medicine & Dentistry, University of Rochester

  This package (LOOS) is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation under version 3 of the License.

  This package is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include <iostream>
#include <fstream>

#include <exceptions.hpp>
#include "LineReader.hpp"

namespace loos {

  std::istream& LineReader::stream() const { return(*_is); }
  void LineReader::stream(std::istream& is) { _is = &is; _lineno = 1; }
    
  std::string LineReader::name() const { return(_name); }
  void LineReader::name(const std::string& name) { _name = name; }



   bool LineReader::getNext() {
    if (! _lines.empty()) {
      _current_line = _lines.back();
      _lines.pop_back();
    } else if (_is->eof())
      return(false);
    else {
      while (getline(*_is, _current_line)) {
        ++_lineno;
        stripComment(_current_line);
        stripLeadingWhitespace(_current_line);
        if (! skipLine(_current_line) )
          break;
      }
    }
    
    checkState();
    return( _is->good() );
  }

   void LineReader::push_back(const std::string& s) {
    _lines.push_back(s);
  }

  std::string LineReader::line() const { return(_current_line); }
  
  unsigned int LineReader::lineNumber() const { return(_lineno); }

  void LineReader::checkState() const {
     if (!(_is->good() || _is->eof())) {

       if (! _name.empty())
         throw(FileReadErrorWithLine(_name, _lineno));
       else
         throw(FileReadErrorWithLine(_lineno));
     }
   }

  void LineReader::stripComment(std::string& s) const {
    if (_comment_char != '\0') {
      std::string::size_type i = s.find('#');
      if (i != std::string::npos)
	s.erase(i, s.length() - i); 
    }
  }
  
  void LineReader::stripLeadingWhitespace(std::string& s) const {
    if (! _leading_chars.empty()) {
      std::string::size_type i = s.find_first_not_of(" \t");
      if (i > 0)
	s.erase(0, i);
    }
  }
  
  bool LineReader::skipLine(const std::string& s) const {
    return(s.empty());
  }
  

}
