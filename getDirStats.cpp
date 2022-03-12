 /*
  + NAME : Ahmad Almasri -- UCID : 30114233
  + 
  + Assignment 2 :: Syscalls (popen,stat,fopen,fget,opendir)
  +
  + This program returns statistical data of a provided directory such as:
  + Largest file and its path
  + Total sum of all files sizes
  + Number of files and directories
  + Most common types of files (determined by n)
  + Most common words in files (determined by n)
  + Duplicate files (determined by n) 
  + This file does not have a main(); therefore, must be called
  + from main().
  +
  +
  +
  */

#include <stdio.h>
#include <string>
#include <vector>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unordered_map>
#include <algorithm>
#include "digester.h"
#include "getDirStats.h"
//===============================================================
#define BUFF_SIZE 1024 
//===============================================================
// TO CHECK IF THE PATH IS DIR
static bool is_dir(const std::string & path){
  struct stat buff;
  if (0 != stat(path.c_str(), & buff)) return false;
  return S_ISDIR(buff.st_mode);
}
//===============================================================
// TO  SEPERATE BY COMMA OR NEWLINE
int getType(char *type){
  int i = 0;
  while(type[i] != ',' && type[i] != 10)
    i++;
  return i;
}
//===============================================================
// FOR HISTOGRAM :: WE NEED TO READ EACH FILE AND SAVE EACH WORD (REQUIREMENT :: LENGTH OF 3)
// THIS FUNCTION RETURNS A STRING FROM AN OPENED FILE
// TO CALL next_word() -> FILE *fp must be initialized and passed
// 1. IT SOTPS WHEN fgetc COUNTERS ANYTHING UNLESS A LETTER
// 2. IT SOTPS WHEN fgetc COUNTERS EOF
// IF SIZE OF THE STRING IS 0 AND fgetc DOES NOT RETURNS A LETTER
// THEN IT CONTINUES READING
std::string next_word(FILE *fp){
  std::string result;
  while(1){
    int c = fgetc(fp);
    if(c == EOF) break;
    c = tolower(c);
    if(! isalpha(c)){
      if(result.size() == 0) continue;
      else break;
    }
    result.push_back(c);
  }
  return result;
}
//===============================================================
// TO SORT HISTOVECTOR && TYPEVECTOR <descendingly>
bool compareVector(const std::pair<std::string, int> &a,
    const std::pair<std::string, int> &b){
  return (a.second > b.second);
}
//===============================================================
// TO SORT HASH <descendingly>
bool compareHash(const std::vector<std::string> &a,
    const std::vector<std::string> &b){
  return a.size() > b.size();
}
//===============================================================
// getDirStats() computes stats about directory a directory
//   dir_name = name of the directory to examine
//   n = how many top words/filet types/groups to report
// if successful, results.valid = true
// on failure, results.valid = false
Results getDirStats(const std::string & dir_name, int n){

  //===============================================================
  // ** check if the provided path is a DIR or NOT
  Results res;
  res.valid = false;
  res.largest_file_size = -1;
  res.largest_file_path = "";
  if (!is_dir(dir_name)) return res;
  //===============================================================
  // ** HISTOGRAM <string, int>
  std::unordered_map<std::string, int> histoMap;
  // ** TYPES <string, int>
  std::unordered_map<std::string, int> typeMap;
  // ** HASH <sring, std::vector>
  std::unordered_map<std::string, std::vector<std::string>> hashMap;
  //===============================================================
  // ** COUNT VARS
  int numOfDir = 0;
  int numOfFile = 0;
  // ** large -> path & size
  std::string largPath = ""; // initial path
  int largSize = -1; // initial data for the size
  // ** total size :: sum of all files
  int totalSize = 0; // total sum of files' size 
  //===============================================================
  // ** TO CALL POPEN AND FOPEN
  FILE *fp;
  char buffer[BUFF_SIZE];
  // ** OPEN & READ & CLOSE : TO ITERATE OVER DIR
  DIR *dir;
  struct dirent *de;
  // ** TO CALL STAT
  struct stat fileStat;
  //===============================================================
  // ** vecotr saves NAMES of all files that we encounter in EACH DIR
  std::vector<std::string> fn; // fn : File Names
  // ** STRING VARS
  std::string name, path, command, type;
  std::string start = dir_name;
  fn.push_back(dir_name);
  bool testDIR = false;
  //===============================================================
  // :: LOOP STARTS ::
  while( ! fn.empty()){
    // ** get the name of the DIR from fn
    std::string dirPath = fn.back();
    // ** remove it from the fn
    fn.pop_back();
    //===============================================================
    // ======= CHECK IF IT IS DIR =======
    testDIR = is_dir(dirPath);
    if( (testDIR) && (dirPath != start) ){
        // ======= counter of DIR =======
        numOfDir++;   
    }else if (dirPath != start){
      // ** IT'S A FILE NOT DIR
      // ======= TYPE OF FILE =======
      command = "file -b  " + dirPath;
      if( (fp = popen(command.c_str(), "r")) == nullptr ){
          printf("Could not call popen()\n");
          return res;
      }
      // ======= read popen =======
      fgets(buffer, BUFF_SIZE, fp);
      // ** close popen()
      pclose(fp);
      // ======= 0 => NULL TERMINATION =======
      buffer[getType(buffer)] = 0;
      type = buffer;
      // ======= TYPEMAP =======
      typeMap[type]++;
      // ======= HISTOGRAM =======
      if( (fp = fopen(dirPath.c_str(), "r")) == nullptr ){
        printf("Could not call fopen()\n");
        return res;
      }
      while(1){
        // ======= read by word =======
        auto w = next_word(fp);
        if(w.size() == 0) break;
        if(w.size() > 2) histoMap[w]++;
      }
     // ** WE CALL STAT TO GET THE SIZE ->
     //     STORE LARGEST FILE SIZE AND PATH
     // ======= call stat() =======
     if(stat(dirPath.c_str(), &fileStat) != 0){
        printf("Could not call the stat()\n");
        return res;
      }else{
        // ======= compare the size of the file =======
        if(fileStat.st_size > largSize){
          largSize = fileStat.st_size;
          largPath = dirPath;
        }
        // ======= total size =======
        totalSize += fileStat.st_size;
      }
      // ======= HASHMAP =======
      hashMap[sha256_from_file(dirPath)].push_back(dirPath);
      // ======= counter of FILES =======
      numOfFile++;
    }
    //===============================================================
    // ** IF IT IS NOT DIR THEN CONTINUE
    if(!testDIR) continue;
    //===============================================================
    // ======= opendir(path) =======
    if( (dir = opendir(dirPath.c_str())) == nullptr ){
      printf("Could not call the opendir()\n");
      return res;
    }else{
      // readdir(dir)
      while(1){
        de = readdir( dir);
        if(!de) break;
        name = de ->d_name;
        // skip the . and .. directories
        if(name == "." || name == "..") continue;
        path = dirPath+"/"+de->d_name;
        // add it to fn
        fn.emplace_back(path);  
      }
      closedir(dir);
    }
  }
  // :: LOOP ENDS ::
  //===============================================================
  // ======= Store Data in the struct Results =======
  res.valid = true;
  res.largest_file_size = largSize;
  res.largest_file_path = largPath;
  res.all_files_size = totalSize;
  res.n_files = numOfFile;
  res.n_dirs = numOfDir;
  //===============================================================
  // ======= TYPEMAP =======
  // ** res.most_common_types
  for(auto &t : typeMap)
    res.most_common_types.emplace_back(t.first, t.second);
  if(res.most_common_types.size() != 0){
  	// ** sort the vector
  	std::sort(res.most_common_types.begin(), res.most_common_types.end(), compareVector);
  	// ** resize the vector
  	res.most_common_types.resize(n);
  }
  //===============================================================
  // ======= HISTOMAP =======
  // ** res.most_common_words
  for(auto &h : histoMap)
    res.most_common_words.emplace_back(h.first, h.second);
  if(res.most_common_words.size() != 0){
  	// ** sort the vector
  	std::sort(res.most_common_words.begin(), res.most_common_words.end(), compareVector);
  	// ** resize the vector
  	res.most_common_words.resize(n);
  }
  //===============================================================
  // ======= HASHMAP =======
  // ** res.duplicate_files
  for(auto &h : hashMap )
    if(h.second.size()>1)
      res.duplicate_files.push_back(h.second);
  if(res.duplicate_files.size() > 1){
  	// ** sort the vector
  	std::sort(res.duplicate_files.begin(), res.duplicate_files.end(), compareHash);
  	// ** resize the vector
  	res.duplicate_files.resize(n);
  }
  //===============================================================

  return res;
}
