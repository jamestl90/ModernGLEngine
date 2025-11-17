/***********************************************************
 *
 * Filename: FileSystem.h
 * Author: James Leupe
 * Purpose: Manage opening and reading files. 
 *
 ***********************************************************/

#ifndef FILESYSTEM_H
#define FILESYSTEM_H
#include "Singleton.h"
#include <string>
#include <map>
#include <vector>
#include <stack>
#include <fstream>
#include <iostream>
#include <algorithm>
#include <sstream>

using std::string;
using std::vector; 
using std::stack;
using std::map;
using std::ifstream;
using std::getline;
using std::stringstream;
using std::ios;

/**
* @class FileSystem
* @brief  A class to handle file i/o. Files can be opened and read line by line.
*	 
* @author James Leupe
* @version 1.0 
* @date 29/02/12
*/
namespace JLEngine
{
	class FileSystem : public Singleton<FileSystem>
	{
	public:
		/** 
		* @brief Default constructor
		* 
		* @param  none
		* @return none 
		*/
		FileSystem();
		/** 
		* @brief loads a file
		* 
		* loads the file given by the parameter
		*
		* @param  the file name
		* @return int, the id of the file or -1 if the file was not found or could not be opened
		*/
		size_t loadFile(string fileName, bool binary = false);
		/** 
		* @brief checks if the file is good
		* 
		* calls the good() function of the file 
		*
		* @param  the id of the file
		* @return bool, true if the file is good, false if the file was not found, or good() returns false
		*/
		bool isGood(int id);
		/** 
		* @brief gets next line in a file 
		* 
		* returns the next line as a vector of strings delimited by delimiter param, ending at end of line character 
		*		  and skipping any line beginning with escapeChar 
		*
		* @param  the file id and the delimiter
		* @return int, the id of the file or -1 if the file was not found or could not be opened
		*/
		vector<string>* getNextLine(int id, char delimiter = ',', char EOLchar = '\n', char escapeChar = '#');
		void getWholeFileAsArray(int id, char* str);
		void getWholeFileString(int id, string& str);
		/** 
		* @brief closes a file
		* 
		* closes a file given an id
		*
		* @param  the id of the file
		* @return bool, true on successful close
		*/
		bool closeFile(int id);
		/** 
		* @brief clears all files
		* 
		* clears the file system of all files
		*
		* @param none
		* @return void
		*/
		void clearFiles();
		/** 
		* @brief clears empty strings
		* 
		* search the vector for empty strings and erases them 
		*
		* @param  a pointer to a vector
		* @return void
		*/
		void removeEmptyStrings(vector<string>& vec);

	private:
		/** 
		* @brief checks if the id is valid
		* 
		* checks if the id is a valid id 
		*
		* @param  the id of the file
		* @return bool, false if id is invalid
		*/
		bool isValidID(int id);
		/** 
		* @brief gets data from file
		* 
		* gets the data from the file using delimiter and end of line character
		*
		* @param  the id of the file, delimiter and end of line identifier
		* @return pointer to a vector of the file data
		*/
		vector<string>* getData(int id, char delimiter, char EOLchar, char escapeChar); 

		/// iterator to the map of files
		map<int, ifstream*>::iterator m_filesIter;
		/// map of files
		map<int, ifstream*> m_files; 
		/// stack to track re-usable id's
		stack<int> m_ids; 
	};
}

#endif