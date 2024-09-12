/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

namespace ecolab
{
  class urand: public random_gen
  {
  public:
    double rand();
    void seed(int s) {srand(s);}
  };

  class gaussrand: public random_gen
  {
    double sum;
    unsigned int n;
    CLASSDESC_ACCESS(gaussrand);
  public:
    urand uni;
    gaussrand() {sum=0; n=0;}
    double rand();
  };
}
