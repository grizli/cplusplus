#include <iostream>
#include <string>
#include <fstream>
#include <map>
#include <cstdlib>
#include <omp.h>
#include <malloc.h>
#include <vector>

using namespace std;

pair<string,vector<char>> getSequence(ifstream& s){
	vector<char> sequence;
	string name;
	int c;

	if(s.get()=='>')
		getline(s,name);

	while (!s.eof()){
		c = s.get(); //get char from reference Sequence file
		if(c == '>'){
			s.unget();
			break;
		}
		else if (c == 'G' || c=='T' || c=='A' || c=='C'){
			sequence.push_back(c);
		}
		// ignore every other chars (newline, etc)
	}

	return make_pair(name,sequence);
}

int main(int argc, char* argv[]){

	// first command line argument : number of worker threads to use
	// not used in this program at this time

	// first command line argument : window size
	
	omp_set_num_threads(1);

	size_t minMatchLength = atol(argv[2]);

	size_t sum=1,i,j,k;

	string result;
	int tnum = atoi(argv[1]);

	// second command line argument : reference sequence file
	ifstream refSeqFile(argv[3],ios::in);
	pair<string,vector<char>> ref = getSequence(refSeqFile);
	string refSeqName = ref.first;
	vector<char> refSeq = ref.second;

	// following command line arguments : other files containing sequences
	// result is stored in an associative array ordered by title
	map<string,vector<char>> otherSequences;
	for(int i=4; i<argc; i++){ // iterate over command arguments
		ifstream seqFile(argv[i],ios::in);
		while(!seqFile.eof()){
			pair<string,vector<char>> other = getSequence(seqFile);
			otherSequences[other.first] = other.second;
		}
	}
	

	// compare other sequences to reference sequence
	// iterate over other sequences
	

	for(map<string,vector<char>>::iterator sequencesIter = otherSequences.begin(); sequencesIter!=otherSequences.end(); sequencesIter++){

		// output sequence name
		vector<char> otherSeq;
		cout << sequencesIter->first << "\n";
		otherSeq = sequencesIter->second;
		// L[i][j] will contain information if substring od ref and other string are equal 
		// starting by positions i in refSeq and j in otherSeq, the information is minimum length or zero
		// in the first comparison inside for
		
		if(minMatchLength>refSeq.size()) continue;
		if(minMatchLength>otherSeq.size()) continue;
		
		size_t **L = new size_t*[refSeq.size()];
		for(i=0; i<refSeq.size();++i)
			L[i] = new size_t[otherSeq.size()];
		

		// here we are setting default value for each cell in array L because we want to minimize reading of that variable (less possible dependencies in parallelization)
		for(i=0;i<refSeq.size()-minMatchLength+1;i++){
	                for(j=0;j<otherSeq.size()-minMatchLength+1;j++){
				L[i][j]=0;
			}
		}
		// here is started parallelization od work
		omp_set_num_threads(tnum);
		// interation over refSeq
		#pragma omp parallel for shared(minMatchLength,otherSeq,refSeq,L) private (i,j,k,sum)
		for(i=0;i<refSeq.size()-minMatchLength+1;i++){
			//iteration over otherSeq
			for(j=0;j<otherSeq.size()-minMatchLength+1;j++){
				if(refSeq[i]==otherSeq[j]){
					//search for minimum criterium		
					for(k=0;k<minMatchLength;k++){
						if(i+k>=refSeq.size() || j+k>=otherSeq.size()) break;
						if((refSeq[i+k]==otherSeq[j+k])){
							sum++;
						} else {
							sum=0;
							break;
						}
					}
					//update L if nessesary, end jump in otherSeq L[i][j] chars
					if(sum>=minMatchLength){
						L[i][j] = sum;
					}
					sum=0;
				}
			}
		}
		
		#pragma omp barrier
		#pragma omp parallel for private (i,j,k) shared (minMatchLength,refSeq,L,otherSeq) ordered
		for(i=0;i<refSeq.size()-minMatchLength;i++){
			for(j=0;j<otherSeq.size()-minMatchLength;j++){
				if(L[i][j]>0){ //if previously we found candidate (minimum criteria) search for more equal chars
					for(k=minMatchLength; (k+i<refSeq.size())&&(k+j<otherSeq.size());k++){
						if((refSeq[i+k]==otherSeq[j+k])){ //if found one, increase L[i][j]
							L[i][j]++;
						}else{ //comparison is finished, inside L[i][j] are results
							break;
						}
					}
				}
			}
		}
		
		#pragma omp barrier

		//here we continue on one thread
		omp_set_num_threads(1);
		
		size_t prev=0;
		string result="";
		char tmp[100]="";
		//we need to print the results on output
		for(i=0;i<refSeq.size()-minMatchLength+1;i++){
			for(j=0;j<otherSeq.size()-minMatchLength+1;j++){
				if(L[i][j]>=minMatchLength){
					//here we eleminate substrings of same identical substring
					//in other words we are loking for longest possible solution
					if (prev == j+L[i][j]+1) continue;
					else{
						sprintf(tmp,"%zd %zd %zd %zd\n",i+1,i+L[i][j],j+1,j+L[i][j]);
						result.append(tmp);
						prev = j+L[i][j]+1;
					}
				}
			}
		}
		
		//freeing memory
		for(size_t i=0; i<refSeq.size();++i)
			delete[] L[i];
		delete[] L;
		
		//printing result
		cout << result;
		
		
	}

	return 0;
}
