#define M80 2
#define M40 3
#define M15 4
#define M10 5
#define NUM_BAND 4

int
make_freq (void)
{
    switch (Roll (NUM_BAND + 2))
      {
      case M80:
	  return (3675 + Roll (50));
	  break;
      case M40:
	  return (7100 + Roll (50));
	  break;
      case M15:
	  return (21100 + Roll (100));
	  break;
      case M10:
	  return (28100 + Roll (200));
	  break;
      default:
	  return (7100 + Roll (50));
	  break;
      }
}
