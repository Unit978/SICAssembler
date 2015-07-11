
#ifndef DYNAMIC_ARRAY_H
#define DYNAMIC_ARRAY_H

#include <iostream>
using std::cout;
using std::endl;

/*
    A wrapper class that handles dynamic allocation
    of memory for the basic array.

    This container keeps the elements in no particular order.

    Resizing:
        Doubles array size if we reached a size >= capacity
        Halves array size if we reached a size <= capacity and size > minCapacity
*/

template <typename T>
class DynamicArray{

    private:
        unsigned numberOfElements;

        //the capacity of the container before
        //there is a need for memory re-allocation
        unsigned capacity;
        const unsigned minCapacity = 16;

        //array container to hold our elements
        T* container;

        //resizes by either expanding or shrinking
        //the array container
        void resize(unsigned newSize){
            //have a pointer to old container
            T* old = container;

            //create new container and update new capacity
            container = new T[newSize];
            capacity = newSize;

            //fill in the new container
            for(int i = 0; i < numberOfElements; i++)
                container[i] = old[i];

            //free old memory
            delete[] old;
        }

        void swap(T& a, T& b){
            T& temp(a);
            a = b;
            b = temp;
        }

    public:
        //construct container
        DynamicArray() :
            numberOfElements(0)
        {
            capacity = minCapacity;
            container = new T[capacity];
        }

        DynamicArray(int capacity):
            numberOfElements(0), capacity(capacity)
        {
            container = new T[capacity];
        }

        //free all memory
        ~DynamicArray(){
            delete[] container;
        }

        void push_front(const T& entry){
            //shift everyone one position forward
            for(int i = numberOfElements-1; i >= 0; i--)
                container[i+1] = container[i];

            //set new value
            container[0] = entry;
            numberOfElements++;

            if(numberOfElements == capacity)
                resize(capacity * 2);
        }

        void push_back(const T& entry){
            //insert at the end
            container[numberOfElements++] = entry;

            //resize if we hit the cap
            if(numberOfElements == capacity)
                resize(capacity * 2);
        }

        void remove(const T& target){
            //find the target
            int i;
            for(i = 0; i < numberOfElements; i++){
                //found
                if(container[i] == target){
                    //swap with last element to simulate a removal
                    swap(container[i], container[numberOfElements-1]);
                    numberOfElements--;

                    //shrink if there is too much spaced unsused
                    if(numberOfElements > minCapacity && numberOfElements <= capacity/4)
                        resize(capacity/2);
                    break;
                }
            }
        }

        //pads the remaining slots in the container to the specified param
        void pad(const T& entry){
            for(int i = numberOfElements; i < capacity-1; i++)
                container[i] = entry;
        }

        //Sets the numberOfElements to the specified param.
        //Param must be smaller than capacity and >= 0
        void forceSize(unsigned size){
            if(size >= 0 && size < capacity)
                numberOfElements = size;
        }

        const T& at(unsigned index) const{
            return container[index];
        }

        unsigned size() const{
            return numberOfElements;
        }

        bool empty() const{
            return numberOfElements == 0;
        }

        void clear(){
            delete[] container;
            capacity = minCapacity;
            container = new T[capacity];
            numberOfElements = 0;
        }

        void display(){
            for(int i = 0; i < numberOfElements; i++)
                cout << container[i] << endl;
        }
};

#endif
