#pragma once
#include "reign/config.hpp"
#include <string>
#include <vector>
namespace reign {
// Camera selection and control activation are separate events in the source.
class ShellMenuNavigation {
public:
 enum class Page { Main, EnteringInstant, Instant, ReturningMain, Launching };
 enum class Event { None, ShowInstant, ShowMain };
 explicit ShellMenuNavigation(const std::vector<ConfigNode> &objectives);
 std::string begin_instant(float now,bool main_camera_finished);
 std::string return_main(float now);
 Event advance(float now);
 void launch();
 void reset();
 Page page()const{return page_;}
 bool main_ready(bool main_camera_finished)const;
private:
 struct Transition {std::string bookmark;float ready;};
 Transition enter_,exit_;
 Page page_=Page::Main;
 float started_=0;
 bool returned_=false;
 static Transition read(const std::vector<ConfigNode> &,const std::string &);
};
}
