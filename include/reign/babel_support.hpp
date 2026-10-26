#pragma once
#include "reign/frontend_support.hpp"
#include <cstddef>
namespace reign::babel_detail {
CH *find(const CH *,CH);
U32 index(const CH *);
void validate(const CH *,U32);
void unknown(CH *,U32,const char *);
void copy(CH *,U32,const CH *,std::size_t);
void copy(CH *,U32,const CH *);
void format(CH *,U32,const CH *,int);
void format(CH *,U32,const CH *,double);
void format(CH *,U32,const CH *,const CH *);
void format(CH *,U32,const CH *,int *);
void format(CH *,U32,const CH *,void *);
}
