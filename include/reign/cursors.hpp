#pragma once
#include <memory>
#include <functional>
class FScope;
namespace reign {
class Cursors {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 Cursors();~Cursors();
 void configure(FScope *);
 unsigned count() const;
 bool select(const char *);
 void draw(int,int,unsigned);
};
}
