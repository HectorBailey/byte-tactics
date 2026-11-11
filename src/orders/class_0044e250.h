#ifndef CLASS_0044E250_H
#define CLASS_0044E250_H

// Class_0044e250: the constructor view of PathOrder that takes a unit and a pad
// index, named after the address of the constructor it models. Its decorated
// name is in data/aliases.csv. The two files that build a PathOrder this way
// share this one view; the base is orders/path_order.h, whose Order and Unit
// forward declarations these files already have in scope. The pad index is int,
// not char: the original loads a full dword.

class Class_0044e250 : public PathOrder {
public:
    Class_0044e250(Order*, Unit*, int);
};

#endif
