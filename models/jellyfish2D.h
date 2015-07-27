/*
  @copyright Russell Standish 2000-2013
  @author Russell Standish
  This file is part of EcoLab

  Open source licensed under the MIT license. See LICENSE for details.
*/

class jellyfish
{
  CLASSDESC_ACCESS(jellyfish);
  void change_dir();
  bool was_in_sunlight;
  bool collision(GraphID_t cell);
public:
  double &x, &y, &vx, &vy, radius;
  array<double> pos, vel;
  int colour; /* 0=black, 1=red, 2=green ... - see jellyfish::draw() */
  unsigned id;
  jellyfish(bool init=false): pos(2), vel(2), x(pos[0]), y(pos[1]), vx(vel[0]), vy(vel[1]) {
    if (init) jinit();
  }
  void jinit();  /* called when you need to initialise a jellyfish */
  void update();
  void draw(eco_string&,int);
  bool operator==(const jellyfish& j) const {return j.x==x && j.y==y&&j.vx==vx&&j.vy==vy&&j.radius==radius&&j.colour==colour&&j.was_in_sunlight==was_in_sunlight;}
  inline bool in_shadow(); 
  GraphID_t mapid();
  GraphID_t mapid_next();
};
