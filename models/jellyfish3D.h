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
  array<double> pos, vel;
  double &x, &y, &z, &vx, &vy, &vz, radius;
  int colour; /* 0=black, 1=red, 2=green ... - see jellyfish::draw() */
  unsigned id;
  jellyfish(bool init=false): pos(3), vel(3), x(pos[0]), y(pos[1]), z(pos[2]),
                              vx(vel[0]), vy(vel[1]), vz(vel[2]) {
    if (init) jinit();
  }
  jellyfish& operator=(const jellyfish& x) {
    pos=x.pos; vel=x.vel; colour=x.colour; id=x.id;
  }
  double& x() {return pos[0];}
  double& y() {return pos[1];}
  double& z() {return pos[2];}
  double& vx() {return vel[0];}
  double& vy() {return vel[1];}
  double& vz() {return vel[2];}
  void jinit();  /* called when you need to initialise a jellyfish */
  void update();
  void draw(eco_string&,int);
  bool operator==(const jellyfish& j) const {return j.x==x && j.y==y&&j.vx==vx&&j.vy==vy&&j.radius==radius&&j.colour==colour&&j.was_in_sunlight==was_in_sunlight;}
  inline bool in_shadow(); 
  inline GraphID_t mapid();
  inline GraphID_t mapid_next();
};
