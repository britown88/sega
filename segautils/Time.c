#include "Time.h"

Microseconds t_m2u(Milliseconds t) {
   return t * 1000;
}
Microseconds t_s2u(Seconds t) {
   return (long)(t * 1000000);
}
Milliseconds t_u2m(Microseconds t) {
   return t / 1000;
}
Milliseconds t_s2m(Seconds t) {
   return (long)(t * 1000);
}
Seconds t_u2s(Microseconds t) {
   return t / 1000000.0;
}
Seconds t_m2s(Milliseconds t) {
   return t / 1000.0;
}