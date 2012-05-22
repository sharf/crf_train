#ifndef SPARSE_VECTOR_HEADER_H
#define SPARSE_VECTOR_HEADER_H

#include<fastlib/fastlib.h>
#include<utility>
#include<iostream>

template <class T>
class SparseVector {

	ArrayList< std::pair<index_t, T> > _data;
	index_t _length;
	
	public:
		void Init();
		void set(index_t i, T value);
		T get(index_t i);
		index_t get_length();
		index_t set_length(index_t l);
		void PrintDebug();
		void Clear(){
			_data.Clear();
			_length = 0;
		}
		GenVector< std::pair<int, T> > get_non_zero_elements(){

			GenVector< std::pair<int, T> > elem;
			elem.Init(_data.size());
			for(int i = 0; i < elem.length(); i++)
				elem[i] = _data[i];
			return elem;
		}

	private:
		index_t get_index_of(index_t i);

};

template <typename T>
void SparseVector<T>::Init(){

	_data.Init();
}

template <typename T>
void SparseVector<T>::set(index_t i, T value){

	//Sensible?
	if(i > _length)
		_length = i+1;

	index_t j = get_index_of(i);
	if(j == -1){
		_data.PushBack();
		_data.back().first = i;
		_data.back().second = value;
	}
	else
		_data[j].second = value;
}

template <typename T>
T SparseVector<T>::get(index_t i){

	if(i > _length - 1)
		return NULL;
	index_t j = 0;
	while(j < _data.size() && _data[j].first != i)
		j++;
	if(j < _data.size())
		return _data[j].second;
	else
		return 0;
}

template <typename T>
index_t SparseVector<T>::get_index_of(index_t i){

	index_t j = 0;
	while(j < _data.size() && _data[j].first != i)
		j++;
	if(j < _data.size())
		return j;
	else
		return -1;
}

template <typename T>
index_t SparseVector<T>::get_length()
{ 
	return _length;
}

template <typename T>
index_t SparseVector<T>::set_length(index_t l)
{ 
	_length = l;
}

template <typename T>
void SparseVector<T>::PrintDebug(){

	std::cerr << "--------- Sparse Vector ----------- " << std::endl;
	for(index_t i = 0; i < _data.size(); i++){
		std::cerr << _data[i].first << "," << _data[i].second << std::endl;	
	}
}

double Dot(const Vector& v, SparseVector<int>& sv){

	GenVector< std::pair<int, int> > non_zero = sv.get_non_zero_elements();
	double dot = 0.0;
	for(int i = 0; i < non_zero.length(); i++) {
		dot += non_zero[i].second * v[non_zero[i].first];
	}
	return dot;
}

void AddTo(SparseVector<int>& sv, Vector* v){

	GenVector< std::pair<int, int> > non_zero = sv.get_non_zero_elements();
	for(int i = 0; i < non_zero.length(); i++)
		(*v)[non_zero[i].first] += non_zero[i].second;

}
#endif
