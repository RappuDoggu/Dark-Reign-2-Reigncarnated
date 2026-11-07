#pragma once
#include <memory>
#include <functional>
class FScope;
namespace reign {
namespace cursor_bridge {extern thread_local std::function<unsigned(unsigned)> lookup;}
class Cursors {
 struct Impl;std::unique_ptr<Impl> impl_;
public:
 Cursors();~Cursors();
 void configure(FScope *);
 void run(const std::function<void()> &);
 unsigned count() const;
 bool select(const char *);
 void draw(int,int,unsigned);
};
}
