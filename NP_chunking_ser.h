#ifndef NP_CHUNKING_HEADER_H
#define NP_CHUNKING_HEADER_H
#include <string.h>
#include <iostream>
#include <map>
#include <utility>
//#include  "fastlib/fastlib.h"
#include "SparseVector.h"

#include <string>
#include <fstream>
#include <iostream>

using namespace std;

#define MAX_LABELS 5
#define MAX_WORDS 100
#define MAX_CHAR 25
#define MAX_SIZE 50
#define MAX_POS_CHAR 10
#define MAX_CHUNK_LABELS 3
#define strnicmp strncasecmp
#define THRESH  2
#define TRAIN_MODE 0
#define TEST_MODE 1
#define TRAIN_AND_TEST 2

int first_zero = 0;

class Featureset 
{
   public:
    map<string, int> Dict_index, Labels_index, POS_index, POS_pair_str_index, Label_pair_str_index, Bigrams_index,Rev_labels_index;
   map<string, int> Rev_unig_index, Rev_POS_index, Rev_big_index, Rev_POS_big_index, Rev_label_big_index;
   map<int, string> Bigram_num_index, Unigram_num_index, Labels_num_index, Labels_pair_index;
   map<int, string> POS_num_index, POS_pair_index;
   int mode;
   FILE * fp;
   char yi[MAX_WORDS][MAX_POS_CHAR];
   char chunk_labels[MAX_CHUNK_LABELS +1][MAX_LABELS];
   char wi[MAX_WORDS][MAX_CHAR+1]; // Input sentence X
   char ti[MAX_WORDS][MAX_POS_CHAR+1];

   int num_words, num_features;
   SparseVector<int> feature_vector;
   //Vector Y, X, T,retY;
   Vector Y, retY;
   int cur_datapoint;
   int thresh;

   public:

   Featureset(){
   	retY.Init(2);
	feature_vector.Init();
   }

   int getX(Vector& X,Vector& T)
   {
     int retval;
     if(cur_datapoint < num_words)
     {
         X[0] = (cur_datapoint > 0) ? Rev_unig_index.find(wi[cur_datapoint-1])->second : -1;
         X[1] = Rev_unig_index.find(wi[cur_datapoint])->second;
         T[0] = (cur_datapoint > 0) ? Rev_POS_index.find(ti[cur_datapoint-1])->second : -1;
         T[1] = Rev_POS_index.find(ti[cur_datapoint])->second;
         cur_datapoint ++;
         return 1;
     }
     else
     {
       retval = get_next_sentence();
       if(retval != EOF)
       { 
         X[0] = (cur_datapoint > 0) ? Rev_unig_index.find(wi[cur_datapoint-1])->second : -1;
         X[1] = Rev_unig_index.find(wi[cur_datapoint])->second;
         T[0] = (cur_datapoint > 0) ? Rev_POS_index.find(ti[cur_datapoint-1])->second : -1;
         T[1] = Rev_POS_index.find(ti[cur_datapoint])->second;
         cur_datapoint ++;
         return 1;

       }
       else
         return EOF;
     }
   }
   
   void setthreshold(int val)
   {
     thresh = val;
   }

   void setmode(int val)
   {
     mode = val;
   }

   int get_next_sentence()
   {
     int retval;
     char word[MAX_SIZE], pos[MAX_SIZE], labels[MAX_SIZE];
     int count = 0;
     char prevlabel[MAX_POS_CHAR]; 
     char curlabel[MAX_POS_CHAR];
     int i;

     strcpy(prevlabel,"O-NP");
     do
     {
       retval = fscanf(fp,"%s %s %s", wi[count], ti[count], curlabel);
       

       if(retval != EOF)  
       {
         //Copy words and pos tags to wi and ti respectively
         if(!strcmp(curlabel,"I-NP") ||
             !strcmp(curlabel,"O-NP") ||
             !strcmp(curlabel,"B-NP") ||
             !strcmp(curlabel,"O"))
          {
            //Let the labels be
          }
          else
          {
            sprintf(curlabel,"O-NP"); // Only considering Noun chunking, converting all other labels to outside NP label
          }

       if(mode == TRAIN_MODE)
       {
         sprintf(yi[count], "%s/%s",prevlabel,curlabel);
         sprintf(prevlabel, "%s",curlabel);
       }
         count++;
         
       }
       else
       {
         cout << "End of file:" << count+1;
         num_words = ++count;
         return EOF;
       }
     }while(wi[count - 1][0] != '.');
      
     num_words = count;
     cur_datapoint = 0; 
     return 1; 
   }

   void print_sentence()
   {
     int i;
     cout << "Printing the sentence : "<< num_words << endl;
     for(i = 0; i < num_words; i++)
     {
       cout << "Word:" << wi[i] << "POS:" << ti[i] ;
       if(mode == TRAIN_MODE)
       {
         cout << "Labels:" << yi[i] << endl;
       }
     }
     cout << "Outside print" << endl;
   }
  
   void go_to_begin(const char *file_name)
   {
      if(fp != NULL)
      { 
        fclose(fp);
      }
      fp = fopen(file_name,"r");
      if(fp == NULL)
      {
        printf("func:go_to_begin:Error when reopening the file: %s \n",file_name);
        exit(1);
      }
   }
  
   void Init(const char *file_name)
   {
      char buf[100];
      char word[MAX_SIZE], pos[MAX_SIZE], labels[MAX_SIZE],curlabel[MAX_SIZE];
      char prevword[MAX_SIZE] = {'\0'}, prevpos[MAX_SIZE] = {'\0'}, prevlabel[MAX_SIZE] = {'\0'},prevlabel_pair[MAX_SIZE] = {'\0'};
      int retval;
      int count;
      // Examples of assigning Map container contents

      sprintf(chunk_labels[0],"B-NP");
      sprintf(chunk_labels[1],"I-NP");
      sprintf(chunk_labels[2],"O-NP");
      sprintf(chunk_labels[3],"O");    
 
      fp = fopen(file_name,"r");
      printf("Reading from the file\n");
      strcpy(prevlabel, "O-NP");
      prevlabel[4] = '\0';

      if(thresh == 0)
      {
         thresh = THRESH;
      }

      if(fp == NULL)
      {
        printf("Error reading from the file\n");
        exit(1);
      }
      else
      {
        do
        {
          retval = fscanf(fp,"%s %s %s", word, pos, curlabel);
          if(prevword[0] != '\0' && retval != EOF)
          {
            sprintf(buf,"%s/%s", prevword, word);
           if(isalnum(prevword[0]) && isalnum(word[0]))
           {
              Bigrams_index[buf]++;
           }
          }
   
          if(prevpos[0] != '\0' && retval != EOF)
          {
            sprintf(buf,"%s/%s", prevpos, pos);
            POS_pair_str_index[buf]++;
          }

          if(!strcmp(curlabel,"I-NP") ||
             !strcmp(curlabel,"O-NP") ||
             !strcmp(curlabel,"B-NP") ||
             !strcmp(curlabel,"O"))
          {
            //Let the labels be
          }
          else
          {
            sprintf(curlabel,"O-NP"); // Only considering Noun chunking, converting all other labels to outside NP label
          }

          if(retval != EOF)
          {
            sprintf(labels, "%s/%s", prevlabel, curlabel);
            sprintf(prevlabel, "%s", curlabel);
            Dict_index[word]++;
            Labels_index[labels]++;
            POS_index[pos]++;
            cout<< "Labels:"<<labels <<endl;
          }
         
          if(prevlabel_pair[0] != '\0' && retval != EOF)
          {
            sprintf(buf,"%s|%s", prevlabel_pair, labels);
            Label_pair_str_index[buf]++;
          }

  
          strncpy(prevword, word, sizeof(word));
          strncpy(prevpos, pos, sizeof(pos));
          strncpy(prevlabel, curlabel, sizeof(curlabel));
          strncpy(prevlabel_pair, labels, sizeof(labels));
        }while(retval != EOF);
      }

      // 1) Assignment using array index notation
      cout << "Map size: " << Dict_index.size() << endl;

      count = 1;
      
      for( map<string, int>::iterator ii= Dict_index.begin(); ii != Dict_index.end(); ++ii,count ++)
      {
        if((*ii).second > thresh*1.0/100*Dict_index.size())
       { Unigram_num_index[count] = (*ii).first;
         Rev_unig_index[(*ii).first] = count -1;
       }
       }
    
      count = 1;
      for( map<string, int>::iterator ii= POS_index.begin(); ii != POS_index.end(); ++ii,count++)
      {
        cout << "POS:" << (*ii).first << ": " << (*ii).second << endl;
         if((*ii).second > thresh*1.0/100*POS_index.size())
       { POS_num_index[count] = (*ii).first;
        Rev_POS_index[(*ii).first] = count -1;  
       }
      }

       count = 1;
       for( map<string, int>::iterator ii= Labels_index.begin(); ii != Labels_index.end(); ++ii,count++)
       {
        cout << "Label:" << (*ii).first << ": " << (*ii).second << endl;
        Labels_num_index[count -1] = (*ii).first;
        cout << "First:" << (*ii).first << "Second:" << (*ii).second <<endl;
         if((*ii).second > thresh*1.0/100*Labels_index.size())
        {Rev_labels_index[(*ii).first] = count -1;
        cout << "Find:"<< Rev_labels_index.find((*ii).first)->second << "Second:"<<Rev_labels_index.find((*ii).first)->first;
        }
       }
        cout << "Labels size:" << Labels_num_index.size();
        cout << "Rev labels size:"<< Rev_labels_index.size();
       count = 1;
      for( map<string, int>::iterator ii= POS_pair_str_index.begin(); ii != POS_pair_str_index.end(); ++ii,count++)
      {
        cout << "Label:" << (*ii).first << ": " << (*ii).second << endl;
         if((*ii).second > thresh*1.0/100*POS_pair_str_index.size())
        {POS_pair_index[count] = (*ii).first;
        Rev_POS_big_index[(*ii).first] = count;
        }
       }
      cout<< "Label_pair_index:" << Label_pair_str_index.size() << endl;
           
      count = 1;
      for( map<string, int>::iterator ii= Label_pair_str_index.begin(); ii != Label_pair_str_index.end(); ++ii,count++)
      {
        cout << "Label pair:" << (*ii).first << ": " << (*ii).second << endl;
         if((*ii).second > thresh*1.0/100*Label_pair_str_index.size())
        {
        Labels_pair_index[count] = (*ii).first;
        Rev_label_big_index[(*ii).first] = count;
        }
      }
      cout << "After print" << endl;

      count = 1;
   
      for(map<string, int>::iterator b = Bigrams_index.begin();b != Bigrams_index.end(); b++,count++)
      {
        cout<< "Bigrams:"<<(*b).first << ":" << (*b).second << endl;
         if((*b).second > thresh*1.0/100*Bigrams_index.size())
        {
          Bigram_num_index[count] = (*b).first;
          Rev_big_index[(*b).first] = count;
        }
      }

      cout<< "Bigrams size:" <<Bigrams_index.size() << endl;


  }

  /*Feature index ranges from 1 - Feature vector size*/
  int find_feature_by_index(int feature_index,int y,int y1, Vector X, Vector T,int i)
  {
    //The input x and y are assumed to be global variables
 
    /*Find out in which set of features the feature_ind falls in to*/

    int num_l = Labels_num_index.size();
    int num_l_pairs = Labels_pair_index.size(); //Will decide later if this is really needed
    int num_c = 3; //For NP-chunking : B, I, O
    int num_unig = Unigram_num_index.size();
    int num_big = Bigram_num_index.size();
    int num_pos = POS_num_index.size();
    int num_pos_pairs = POS_pair_index.size();
    int p_pred = predicate_p(feature_index, y, y1, X, i);
    int q_pred = predicate_q(feature_index, y, y1, X, i);
    return (p_pred * q_pred != 0);
  }

  int predicate_q(int feature_index, int yind, int y1ind, Vector X, int i)
  {
 
    /*Find out in which set of features the feature_ind falls in to*/
    feature_index++;

    int num_l = Labels_num_index.size();
    int num_l_pairs = Labels_pair_index.size(); 
    int num_c = 3; //For NP-chunking : B, I, O
    int num_unig = Unigram_num_index.size();
    int num_big = Bigram_num_index.size();
    int num_pos = POS_num_index.size();
    int num_pos_pairs = POS_pair_index.size();
    const char *w = NULL , *w1 = NULL , *t = NULL, *t1 = NULL, *y = NULL, *y1 = NULL, *pair = NULL;
    int temp_index, yindex, cindex;
    char buf[MAX_SIZE];
    const char *y_i = NULL, *y_i1 = NULL;
    char tempy[MAX_SIZE];

  
    y_i = Labels_num_index.find(yind)->second.c_str();
    y_i1 = Labels_num_index.find(y1ind)->second.c_str();
    sprintf(tempy, "%s", y_i);   
 
    if(feature_index <= num_l)
    {
      //Return the label in the feature_index position
      y = Labels_num_index.find(feature_index -1)->second.c_str();
      return(strnicmp(y, y_i, strlen(y_i)) == 0);
    }
    else if(feature_index <= num_l + num_l_pairs)
    {
      if((i -1) >= 0)
      {
        //Return the label pair from the index
        temp_index = feature_index - num_l;
        sprintf(buf,"%s",Labels_pair_index.find(temp_index)->second.c_str());
        y = strtok(buf, "|");
        y1 = strtok(buf, "|");
        return ( (0 == strnicmp(y, y_i, strlen(y_i))) &&
                 (0 == strnicmp(y, y_i1, strlen(y_i1))));
      }
      else
      {
        return 0;
      }
    }
    else if(feature_index <= num_l + num_l_pairs + num_c)
    {
      char *c;
      // Calculate index to map from feature_index
      temp_index = feature_index - (num_l + num_l_pairs);
      c = strtok(tempy, "/");
      return(!strnicmp(chunk_labels[temp_index - 1], c,sizeof(chunk_labels[temp_index -1]))); // Check if second label of yi is same c
    }
    else
    {
      //Calculate index to map from feature_index
      char *c;
      int res;
      temp_index = feature_index - (num_l + num_l_pairs + num_c);
      temp_index /= (4*num_unig + 2*num_big + num_pos + 2*num_pos_pairs); 
      yindex = temp_index/num_c;
      cindex = (temp_index%num_c); //Indexing into array so no + 1 required
      if(yindex < Labels_num_index.size())
      {
      sprintf(buf,"%s",Labels_num_index.find(yindex)->second.c_str());;
      c = strtok(buf,"/");
      c = strtok(buf, "/");
      y = Labels_num_index.find(yindex)->second.c_str();
      res= !strnicmp(y, y_i, strlen(y_i)) &&
              !strnicmp(chunk_labels[cindex], c, sizeof(chunk_labels[cindex]));
      }
      else
      {
        return 0; 
      }
    }
  }

  int predicate_p(int feature_index, int yind, int y1ind, Vector X, int i)
  {
    int num_l = Labels_num_index.size();
    int num_l_pairs = Labels_pair_index.size(); 
    int num_c = 3; //For NP-chunking : B, I, O
    int num_unig = Unigram_num_index.size();
    int num_big = Bigram_num_index.size();
    int num_pos = POS_num_index.size();
    int num_pos_pairs = POS_pair_index.size();
    const char *w = NULL , *w1 = NULL , *t = NULL, *t1 = NULL, *y = NULL, *y1 = NULL, *pair = NULL;
    int temp_index, yindex, cindex;
    char buf[MAX_SIZE];

    if(feature_index <= (num_l + num_l_pairs + num_c))
    {
      return 1; // p is true for this range of features
    }
    else
    {
      // Fit the feature index to one of the below ranges

      feature_index -= (num_l + num_l_pairs + num_c);

      if(feature_index <= num_unig)
      {
        w = Unigram_num_index.find(feature_index)->second.c_str();
        return(!strnicmp(w,wi[i],sizeof(wi[i])));
      }
      else if(feature_index <= 2*num_unig)
      {
        if((i -1) > 0)
        {
          temp_index = feature_index - num_unig;
          w = Unigram_num_index.find(temp_index)->second.c_str();
          return(!strnicmp(w,wi[i - 1],sizeof(wi[i -1])));
        }
        else
        {
          return 0;
        }
      }
      else if(feature_index <= 3*num_unig)
      {
        if( (i+1) < num_words)
        {
          feature_index = feature_index - 2*num_unig;
          w = Unigram_num_index.find(feature_index)->second.c_str();
          return(!strnicmp(w,wi[i+1],sizeof(wi[i +1])));
        }
        else
        {
          return 0;
        }

      }
      else if(feature_index <= 4*num_unig)
      {
        if( (i-2) > 0)
        {
          feature_index = feature_index - 3*num_unig;
          w = Unigram_num_index.find(feature_index)->second.c_str();
          return(!strnicmp(w,wi[i-2],sizeof(wi[i -2])));
        }
        else
        {
          return 0;
        }
      }
      else if(feature_index <= 4*num_unig + num_big)
      {
        if((i -1) > 0)
        {
          feature_index -= (4*num_unig);
           sprintf(buf, "%s", Bigram_num_index.find(feature_index)->second.c_str());
           w = strtok(buf, "/");
           w1 = strtok(buf, "/");

          return(!strnicmp(w,wi[i-1],sizeof(wi[i-1])) &&
                 !strnicmp(w1,wi[i],sizeof(wi[i])));
        }
        else
        {
          return 0;
        }
      }
      else if(feature_index <= 4*num_unig + 2*num_big)
      {
        if((i+1) < num_words)
        {
          feature_index -= (4*num_unig + num_big);
          sprintf(buf,"%s",Bigram_num_index.find(feature_index)->second.c_str());
          w = strtok(buf, "/");
          w1 = strtok(buf, "/");

          return(!strnicmp(w, wi[i], sizeof(wi[i])) &&
                 !strnicmp(w, wi[i+1], sizeof(wi[i+1])));
        }
        else
        {
          return 0;
        }
      }
      else if(feature_index <= 4*num_unig + 2*num_big + num_pos)
      {
        feature_index -= (4*num_unig + 2*num_big);
        t = POS_num_index.find(feature_index)->second.c_str();
        return (!strnicmp(t, ti[i], sizeof(ti[i])));
      }
      else if(feature_index <= 4*num_unig + 2*num_big + num_pos + num_pos_pairs)
      {
        if((i+1) < num_words)
        {
          feature_index -= (4*num_unig + 2*num_big + num_pos);
          sprintf(buf, "%s",  POS_pair_index.find(feature_index)->second.c_str());
          t = strtok(buf, "/");
          t1 = strtok(buf, "/");
          return(!strnicmp(t, ti[i], sizeof(ti[i])) &&
                 !strnicmp(t1, ti[i+1], sizeof(ti[i+1])));
        }
        else
        {
          return 0;
        }
      }
      else if(feature_index <= 4*num_unig + 2*num_big + num_pos + 2*num_pos_pairs)
      {
        if((i-1) >= 0)
        {
          feature_index -= (4*num_unig + 2*num_big + num_pos + num_pos_pairs);
          sprintf(buf, "%s", POS_pair_index.find(feature_index)->second.c_str());
          t = strtok(buf, "/");
          t1 = strtok(buf, "/");
          return(!strnicmp(t, ti[i-1], sizeof(ti[i])) &&
                 !strnicmp(t, ti[i], sizeof(ti[i-1])));
        }
        else
        {
          return 0;
        }
      }
      else
      {
        return 0;
      }
    }
  }

  int get_num_features()
  {
    int num_l = Labels_num_index.size();
    int num_l_pairs = Labels_pair_index.size(); 
    int num_c = 3; //For NP-chunking : B, I, O
    int num_unig = Unigram_num_index.size();
    int num_big = Bigram_num_index.size();
    int num_pos = POS_num_index.size();
    int num_pos_pairs = POS_pair_index.size();

    int num_q_predicates = (num_l + num_l_pairs + num_c + num_l * num_c);
    int num_p_predicates = (4*num_unig + 2*num_big + num_pos + 2*num_pos_pairs); 
    num_features = num_l + num_l_pairs + num_c + (num_l * num_c * num_p_predicates);
    return (num_features);
  }

   SparseVector<int>& get_feature_vector(int y, int y1, Vector X, Vector T,int i)
  {
    int num_l = Labels_num_index.size();
    int num_l_pairs = Labels_pair_index.size(); 
    int num_c = 3; //For NP-chunking : B, I, O
    int num_unig = Unigram_num_index.size();
    int num_big = Bigram_num_index.size();
    int num_pos = POS_num_index.size();
    int num_pos_pairs = POS_pair_index.size();
    int temp_index, yindex, cindex;
    int non_zero_features;
    GenVector<int> non_zero_indices;

    int num_q_predicates = (num_l + num_l_pairs + num_c + num_l * num_c);
    int num_p_predicates = (1+ 4*num_unig + 2*num_big + num_pos + 2*num_pos_pairs);
    int k,j;

    int num_features = get_num_features();

    feature_vector.Clear();
    feature_vector.set_length(num_features);

    // Find out the non zero Indices
    non_zero_features = Find_non_zero_indices(non_zero_indices, y, y1, X, T, i);    
   
 
    for(k =0; k < non_zero_features; k++)
    {
      feature_vector.set(non_zero_indices[k],1); 
    }

    return feature_vector;
  }
  
  int Find_non_zero_indices(GenVector<int> &non_zero_features, int y, int y1, Vector X, Vector T, int i)
  {
    const char * y_i = NULL, *y_i1 =NULL;
    int fcount = 0;
    char bigram[MAX_SIZE];
    int num_l = Labels_num_index.size();
    int num_l_pairs = Labels_pair_index.size();
    int num_c = 4; //For NP-chunking : B, I, O
    int num_unig = Unigram_num_index.size();
    int num_big = Bigram_num_index.size();
    int num_pos = POS_num_index.size();
    int num_pos_pairs = POS_pair_index.size();
    char chunk_label[20];
    char *c;
    char word_bigram[MAX_SIZE];
    char pos_bigram[MAX_SIZE];
    int second_q_pred;
 
    non_zero_features.Init(13); //Number of features that will be non zero
  
    y_i = Labels_num_index.find(y)->second.c_str();
    y_i1 = Labels_num_index.find(y1)->second.c_str();
    sprintf(chunk_label,"%s",y_i);
    // y = y'
   non_zero_features[fcount] = y;
   // yi = y, yi-1 = y'
   sprintf(bigram,"%s|%s",y_i,y_i1);
 
   if(i > 0 && Rev_big_index.find(bigram)->second != 0)
   {
     non_zero_features[++fcount] = num_l + Rev_big_index.find(bigram)->second; 
   }
   //c(yi) = c
   c = strtok(chunk_label,"/");
   int c_index;
   
   switch(c[0])
   {
     case 'O':
     {
       if(c[1]!= '\0')
        c_index = 2;
       else
        c_index = 3;
     }
     break;
     case 'B':
      c_index = 0;
     break;
     case 'I':
      c_index = 1;
     break;
   }
   non_zero_features[++fcount] = num_l + num_l_pairs + c_index; 

   // yi = y or c(y_i) = c 
   if(Rev_labels_index.find(y_i)->second != 0) {
     second_q_pred = num_l + num_l_pairs + num_c + Rev_labels_index.find(y_i)->second * (4*num_unig + 2*num_big + num_pos + 2*num_pos_pairs)
                                   + c_index;
   // wi = w
   
   if(Rev_unig_index.find(wi[i])->second != 0)
     non_zero_features[++fcount] = second_q_pred + Rev_unig_index.find(wi[i])->second;
   //wi-1 = w
   if(i > 0 && Rev_unig_index.find(wi[i-1])->second != 0)
     non_zero_features[++fcount] = second_q_pred + num_unig + Rev_unig_index.find(wi[i-1])->second;
   //wi+1 = w
   if(i+1 < num_words && Rev_unig_index.find(wi[i+1])->second != 0)
     non_zero_features[++fcount] = second_q_pred + 2*num_unig + Rev_unig_index.find(wi[i+1])->second;
   //wi+2 = w
   if(i+2 < num_words && Rev_unig_index.find(wi[i+2])->second != 0)
    non_zero_features[++fcount] = second_q_pred + 3*num_unig + Rev_unig_index.find(wi[i+2])->second;
   //wi = w'
    if(Rev_unig_index.find(wi[i])->second != 0)
    non_zero_features[++fcount] = second_q_pred + 4*num_unig + Rev_unig_index.find(wi[i])->second;
   //wi = w', wi+1 = w
   if(i+1  < num_words)
   {
     sprintf(word_bigram,"%s/%s",wi[i],wi[i+1]);
     if( Rev_big_index.find(word_bigram)->second != 0 )
       non_zero_features[++fcount] =  second_q_pred + 4*num_unig + Rev_big_index.find(word_bigram)->second;
     
   }
   //wi = w', wi-1 = w
   if(i-1 < num_words)
   {
     sprintf(word_bigram, "%s/%s", wi[i-1], wi[i]);
     if(Rev_big_index.find(word_bigram)->second != 0)
       non_zero_features[++fcount] = second_q_pred + 4*num_unig + num_big  + Rev_big_index.find(word_bigram)->second;
   }
   //ti = t'
    non_zero_features[++fcount] =  second_q_pred + 4*num_unig + 2 * num_big + Rev_POS_index.find(ti[i])->second;
   //ti = t' ti+1 = t
   if(i + 1 < num_words)
   {
    sprintf(pos_bigram,"%s/%s", ti[i], ti[i+1]);
    if(Rev_POS_big_index.find(ti[i+1])->second != 0)
      non_zero_features[++fcount] = second_q_pred + 4*num_unig + 2 * num_big + num_pos + Rev_POS_big_index.find(ti[i+1])->second;
   }
   //ti = t', ti-1 = t
   if( i > 0)
   {
     sprintf(pos_bigram,"%s/%s", ti[i],ti[i-1]);
     if(Rev_POS_big_index.find(pos_bigram)->second != 0)
       non_zero_features[++fcount] =   second_q_pred + 4*num_unig + 2* num_big + num_pos + num_pos_pairs 
                                                  + Rev_POS_big_index.find(pos_bigram)->second;
   }
  }
   return fcount;
  }


   Vector& getY()
   {
     int i;

     if(mode == TRAIN_MODE)
     {
       if(cur_datapoint == 1)
       {
       
         Y.Init(num_words);
         for(i = 0; i < num_words; i++)
         {
           Y[i] = Rev_labels_index.find(yi[i])->second;
         }
         for( map<string, int>::iterator ii= Rev_labels_index.begin(); ii != Rev_labels_index.end(); ++ii)
         {
         }
         retY[0] = Y[0];
         retY[1] = Y[1];
        }
        else
        {
         retY[0] = Y[cur_datapoint -1];
         retY[1] = Y[cur_datapoint -1];
        }

       return retY; 
     }   
   }
   
   int get_num_labels()
   {
     return Rev_labels_index.size();
   }

 
	template <class U, class V>
	void persist_map(const map<U, V>& m, const char* filename){

		ofstream f;
		f.open(filename);
		typename map<U, V>::const_iterator it;
		for(it = m.begin(); it != m.end(); it++) {
			f << (*it).first << std::endl;
			f<<  (*it).second << std::endl;
		}
		f.close();
	}

	template <class U, class V>
	void load_map(map<U, V>& m, const char* filename){

		ifstream f;
		f.open(filename);
		typename map<U, V>::iterator it;
		U tmpU;
		V tmpV;
		m.clear();
		while(f.good()){
			f >> tmpU;
			f >> tmpV;
			m[tmpU] = tmpV; 
		}
		f.close();

	}

	void save(const char** filenames) {

		persist_map(Dict_index, filenames[0]);	
		persist_map(Labels_index, filenames[1]);	
		persist_map(POS_index, filenames[2]);	
		persist_map(POS_pair_str_index, filenames[3]);	
		persist_map(Label_pair_str_index, filenames[4]);	
		persist_map(Bigrams_index, filenames[5]);	
		persist_map(Rev_labels_index, filenames[6]);	
		persist_map(Rev_unig_index, filenames[7]);	
		persist_map(Rev_POS_index, filenames[8]);	
		persist_map(Rev_big_index, filenames[9]);	
		persist_map(Rev_POS_big_index, filenames[10]);	
		persist_map(Rev_label_big_index, filenames[11]);	

		persist_map(Bigram_num_index, filenames[12]);
		persist_map(Unigram_num_index, filenames[13]);
		persist_map(Labels_num_index, filenames[14]);
		persist_map(Labels_pair_index, filenames[15]);
		persist_map(POS_num_index, filenames[16]);
		persist_map(POS_pair_index, filenames[17]);
	}
	
	void load(const char** filenames){

		load_map(Dict_index, filenames[0]);	
		load_map(Labels_index, filenames[1]);	
		load_map(POS_index, filenames[2]);	
		load_map(POS_pair_str_index, filenames[3]);	
		load_map(Label_pair_str_index, filenames[4]);	
		load_map(Bigrams_index, filenames[5]);	
		load_map(Rev_labels_index, filenames[6]);	
		load_map(Rev_unig_index, filenames[7]);	
		load_map(Rev_POS_index, filenames[8]);	
		load_map(Rev_big_index, filenames[9]);	
		load_map(Rev_POS_big_index, filenames[10]);	
		load_map(Rev_label_big_index, filenames[11]);	

		load_map(Bigram_num_index, filenames[12]);
		load_map(Unigram_num_index, filenames[13]);
		load_map(Labels_num_index, filenames[14]);
		load_map(Labels_pair_index, filenames[15]);
		load_map(POS_num_index, filenames[16]);
		load_map(POS_pair_index, filenames[17]);
	}
	
};

namespace boost {
namespace serialization {

template<class Archive>
void serialize(Archive & ar, Featureset & fs, const unsigned int version)
{
   ar & fs.Dict_index;
   ar & fs.Labels_index;
   ar & fs.POS_index;
   ar & fs.POS_pair_str_index;
   ar & fs.Label_pair_str_index;
   ar & fs.Bigrams_index;
   ar & fs.Rev_labels_index;
   ar & fs.Rev_unig_index;
   ar & fs.Rev_POS_index;
   ar & fs.Rev_big_index;
   ar & fs.Rev_POS_big_index;
   ar & fs.Rev_label_big_index;
   ar & fs.Bigram_num_index;
   ar & fs.Unigram_num_index;
   ar & fs.Labels_num_index;
   ar & fs.Labels_pair_index;
   ar & fs.POS_num_index;
   ar & fs.POS_pair_index;
}

} 
} 
#endif
