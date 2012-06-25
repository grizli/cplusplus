/*
 * extractor.cpp
 *
 *  Created on: Jun 17, 2012
 *      Author: ivan
 */

#include <vector>
#include <string>
#include <archive.h>
#include <archive_entry.h>
#include <boost/filesystem.hpp>
#include <iostream>
#include <cstring>

using namespace std;
using namespace boost::filesystem;

class Extractor{
  struct archive *ar,*ext;
  struct archive_entry *entry;
  int r;
  int64_t entry_size;
  string subdir,parent;
  char *filename;
  const char *newfilename,*name;

  protected:
    vector< string > files;

  public:
    /*
     * This method is "universal" extractor modified for debian packadges
     */
    int loadUnpackArchive(int deb){
    	Extractor *secondExtractor;
    	int flags,i,code=0;

    	const char *filename;

    	this->ar = archive_read_new();
    	archive_read_support_format_all(this->ar);
    	archive_read_support_compression_all(this->ar);
    	r = archive_read_open_filename(this->ar, this->filename, 10240);
    	if (r != ARCHIVE_OK)
    		return 1;
       	flags = ARCHIVE_EXTRACT_TIME;
    	flags |= ARCHIVE_EXTRACT_PERM;
    	flags |= ARCHIVE_EXTRACT_ACL;
    	flags |= ARCHIVE_EXTRACT_FFLAGS;

    	/*
    	 * with another archive files and folders will be written to root folder
    	 */
    	this->createSetWorkingDir();
    	this->ext = archive_write_disk_new();
    	archive_write_disk_set_options(this->ext, flags);
    	archive_write_disk_set_standard_lookup(this->ext);

    	i=0;
		while (archive_read_next_header(this->ar, &entry) == ARCHIVE_OK) {
			/*
			 * while ar archive has files (its normal to be 3, and first is always debian-binary
			 * second and third must bi control and data
			 * othervise it isn't debian packadge
			 */
			if(deb==0 && i==3) return 1;
			filename = archive_entry_pathname(entry);
			if(deb==0 && i==0 && strcmp("debian-binary",filename)!=0) return -1;
			this->newfilename = this->setFilename(filename,deb);
			if (deb==0 && i>0){
				this->files.push_back(string(this->newfilename));

			}
			if (archive_entry_size(entry) > 0) {
    			r = archive_write_header(this->ext, entry);
    			if (r != ARCHIVE_OK)
    			    return 1;
    			r = this->copy_data();
    			if (r!=0) return r;
			this->deleteNameFilename();
    		}
    		i++;
    	}
    	r = archive_read_close(this->ar);
    	if (r != ARCHIVE_OK)
    		return 1;
    	r = archive_write_finish_entry(ext);
    	archive_write_close(ext);
    	if (deb==0 && this->files.size()>0){
    		/*
    		 * here are two recursve calls to extract other two subarchives
    		 * previously extracted from ar archive.
    		 * Because gouping into folders is needed, environment location
    		 * is changing with createSetWorkingDir() above and with chdir()
    		 * after extraction
    		 */

    		for(vector< string >::iterator iter = this->files.begin();iter<this->files.end();iter++){
    			secondExtractor = new Extractor();
    			secondExtractor->setSource((char *)(*iter).c_str());

			      /*
			       * Checking that control and data exist
			       */
			      if(((*iter).substr(0,(*iter).find('.'))).compare("data")!=0){
				      code=code+3;
			      }else if(((*iter).substr(0,(*iter).find('.'))).compare("control")!=0){
				      code=code+5;
			      }else{
				      code=10; //any number bigger than 5+3
			      }

			      if(iter==this->files.end()-1 && code!=8){ //if third file in ar archive is now check this rule
				      return 1;
			      }

    			r = secondExtractor->loadUnpackArchive(1);
    			delete(secondExtractor);
    			chdir("../");
    			remove_all((*iter).c_str());
			if (r!=0) return 1;

    		}
    	}

    	return 0;
    };

    int copy_data(){
    	/*
    	 * This method copies files from one archive to another "archive"
    	 * -> "archive" is written to its root folder as files and folders
    	 */
    	int r;
    	const void *buff;
    	size_t size;
    	off_t offset;

    	for (;;) {
    		r = archive_read_data_block(this->ar, &buff, &size, &offset);
    		if (r == ARCHIVE_EOF)
    			return 0; //(ARCHIVE_OK);
    		if (r != ARCHIVE_OK)
    			return (r);
    		r = archive_write_data_block(this->ext, buff, size, offset);
    		if (r != ARCHIVE_OK) {
    			return (r);
			}
		}
    };


    /*
     * If error occur in extracting this delete everything what was created
     *    * * * BE AWARE: this is like rm -r <name> in shell * * *
     */
    void deleteDirectoryRmR(const char* name){
	chdir("../");
	if(strcmp(name,"/")==0) exit(1);
	if(strcmp(name,"./")==0) exit(1);
    	remove_all(name);
    };

    const char* setFilename(const char *path, int deb){
    	/*
    	 * this is helper method for select full paths for extraction
    	 */
    	string str=string(path),str2,filename;
    	const char *name;
    	int at;
    	at=str.find('.');
    	str2=str.substr(0,at);
    	if(str2.compare("debian-binary")==0){
    		filename = str;
    	}else{
    		if(deb!=0){
    			filename = this->parent + str.substr(2,str.size());
    		}else filename = str;
    	}
    	name = new char(filename.size()+1);
    	name = filename.c_str();
    	return name;
    }
    void setSource(char *filename){
    	/*
    	 * Saving root (parent) directory and filename of archive
    	 */
    	string str=string(filename);
    	int at;
    	at=str.find('.');
    	this->parent = string(str.substr(0,at)) + "/";
    	this->filename=filename;
    }
    char* getFilename(){
    	return this->filename;
    }
    void createSetWorkingDir(){
    	/*
    	 * This method creates subdirs and make them as default
    	 * environment location (it's simplifying setFilename() method)
    	 */
    	create_directory(this->parent.c_str());
    	chdir(this->parent.c_str());
    }
    const char* getParent(){
    	return this->parent.c_str();
    }
    void deleteNameFilename(){
	delete[] name;
    }
    ~Extractor(){
	if(sizeof(this->name)>1) this->deleteNameFilename();
    }
};

/*
 * remove_all
 * create_directory
 */

int main (int argc, char *argv[])
{
    int i,r;
    Extractor *ext;

    if(argc<2){
	cout << "Usage: program <debian packadge> [<debian packadge>]" << endl;
	return 1;
    }

    for(i=1;i<argc;i++){ //multiple inputs
    r=0;
    ext = new Extractor();
    /*
     * this prints to standard out name of debian packadge
     * Note: if it's non-debian packadge, then program will end at first
     * check of content of Ar archive and clean
     */
    cout << argv[1] << endl;
    ext->setSource(argv[i]);
    r = ext->loadUnpackArchive(0);
    if (r!=0){
    	/*
    	 * if error occurs inform user
    	 */
    	cout << "Error in extracting debian type archive!" <<endl;
    	cout << "Purging new files end directories it they are created..." << endl;
    	cout << ext->getParent() << endl;
    	ext->deleteDirectoryRmR(ext->getParent());
    	cout << "Task is done.Exiting." << endl;
	delete(ext);
	return 1;
    }else cout << "Extracted!" << endl;
		/*
		 * if all is ok inform user
		 */

    delete(ext);
    /*
     * before continuing loading another archive environment path must be the initial path
     */
    chdir("../");
    }

    return 0;
}
