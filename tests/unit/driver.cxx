#include <pest/pest.hxx>

namespace {

emptyspace::pest::suite basic( "pest suite", []( auto& expect ) {} );

}

int main() {
  basic( std::clog );
  return 0;
}
