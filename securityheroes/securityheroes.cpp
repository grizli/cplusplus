
#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <map>
#include<stdlib.h>

using namespace std;

pair<unsigned int,vector<short int> > getSequence(ifstream& s){
	vector<short int > sequence;
	unsigned int num=0,i;
	short int number;

	char size[4],tmp_number[2];

	s.readsome(size,4);
	//read how many pair bytes are there for read
	num = 0x00 | ((unsigned int)size[0]<<0) | ((unsigned int)size[1]<<8) | ((unsigned int)size[2]<<16) | ((unsigned int)size[3]<<24);

	//and read them
	for(i=0;i<num;i++){
		s.readsome(tmp_number,2);
		number = 0x00 | (tmp_number[0]<<0) | (tmp_number[1]<<8);
		if (number>=0) sequence.push_back(number);
	}
	//return number of 2-byte elements in file, and vector which contains nonnegative numbers
	return make_pair(num,sequence);

}

float arith_mean(vector<short int> content){
	unsigned int sum=0;
	unsigned int size;
	size = content.size();

	//sumarize all elements in given vector
	for(vector<short int>::iterator iter = content.begin(); iter<content.end();iter++){
		sum+=*iter;
	}
	//and return arithmetic mean
	return (float)sum/size;
}

int findPlace(vector<short int> number, int num){
	//find on which place is number 'num'
	//othervise return -1
	unsigned int i;
	for(i=0;i<number.size();i++){
				if(number[i]==num) return i;
	}
	return -1;
}

vector<short int> mode(vector<short int> content){
	vector<short int> number,freq,modes;
	int position;
	unsigned int i,j;
	short int tmp;

	//for each number in vector 'content' calculate its frequency
	for(vector<short int>::iterator iter=content.begin();iter<content.end();iter++){
		position = findPlace(number,(*iter));
		if(position==-1){
			//number doesn't exists in vector 'number' -> insert it and frequency is 1 (vector '1') for first seen
			number.push_back(*iter);
			freq.push_back(1);
		}else{
			//number already exists in vector 'number' -> incerease counter
			freq[position]++;
		}
	}

	tmp=0;

	for(vector<short int>::iterator iter=freq.begin();iter<freq.end();iter++){
		if(tmp<(*iter)){
			tmp=(*iter);
		}
	}

	for(i=0 ;i<freq.size()-1;i++){
		if(tmp==freq[i]){
			modes.push_back(number[i]);
		}
	}

	for(i=0; i<modes.size()-1;i++)
		for(j=0; j<modes.size()-1;j++){
			if(modes[i] < modes[j]){
					tmp = modes[i];
					modes[i] = modes[j];
					modes[j] = tmp;
		}
	}

	return modes;
}

short int median(vector<short int> content){
	short int med;
	short int tmp;

	//sort vector
	for(vector<short int>::iterator iter2 = content.begin(); iter2<content.end();iter2++)
	for(vector<short int>::iterator iter = content.begin(); iter<content.end();iter++){
		if((*iter2)<(*iter)){
			tmp = *iter;
			(*iter) = (*iter2);
			(*iter2) = tmp;
		}
	}

	//return median
	if(content.size()-1%2==0){
		med = content[(content.size()/2)-1];
	}else{
		med = content[((content.size()+1)/2)-1];
	}
	return med;
}



int main(int argc, char **argv) {
	ifstream refSeqFile(argv[1], ios::in | ios::binary);
	unsigned int size;
	vector<short int> content,modes;
	float mean;
	short int med;

	pair<unsigned int,vector<short int> > sequence = getSequence(refSeqFile);
	size = sequence.first;
	content = sequence.second;

	mean = arith_mean(content);
	med = median(content);
	modes = mode(content);

	cout << "Total data count:       " << size << endl;
	cout << "Total valid data count: " << content.size() << endl;;
	cout << "* * * * * * * * * * * * * * * * " << endl;
	cout << "mean:   " << mean << endl;
	cout << "median: " << med << endl;
	cout << "mod:    ";
	for(vector<short int>::iterator iter=modes.begin();iter<modes.end(); iter++){
		cout <<*iter;
		if(iter!=modes.end()-1) cout <<", ";
	}

	return 0;
}
