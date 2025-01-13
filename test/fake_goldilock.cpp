#include <iostream>

int main(){
  auto val = std::getenv("GOLDILOCK_CRASH");
  if(val != nullptr) {
    throw std::runtime_error("Error");
  }

  std::cout<<"goldilock v0.0.1 (built from abcdefg)"<<std::endl;

    
  
  return 0;
}