#include <stdio.h>

void
PutForm0 (void)
{
    PutRST ();
    PutName ();
    PutLocation ();
    PutMisc ();
    PutRig ();
    PutWeather ();
    PutJob ();
    PutAge ();
    PutMisc ();
    PutQ_And_Freq ();
    PutLicense ();
}

void
PutForm1 (void)
{
    PutLocation ();
    PutRST ();
    PutRig ();
    PutWeather ();
    PutMisc ();
    PutName ();
    PutLicense ();
    PutMisc ();
    PutQ_And_Freq ();
    PutAge ();
    PutJob ();
}

void
PutForm2 (void)
{
    PutThanks ();
    PutRST ();
    PutName ();
    PutWeather ();
    PutLocation ();
    PutJob ();
    PutLicense ();
    PutRig ();
    PutAge ();
    PutQ_And_Freq ();
}

void
PutForm3 (void)
{
    PutLocation ();
    PutRST ();
    PutRig ();
    PutMisc ();
    PutName ();
    PutMisc ();
    PutAge ();
    PutJob ();
    PutLicense ();
    PutMisc ();
    PutWeather ();
    PutMisc ();
    PutQ_And_Freq ();
}

void
PutForm4 (void)

{
    PutThanks ();
    PutRST ();
    PutJob ();
    PutMisc ();
    PutMisc ();
    PutName ();
    PutAge ();
    PutLicense ();
    PutRig ();
    PutLocation ();
    PutWeather ();
    PutMisc ();
}

void
PutForm5 (void)
{
    PutLocation ();
    PutRST ();
    PutRig ();
    PutName ();
    PutJob ();
    PutAge ();
    PutMisc ();
    PutLicense ();
    PutWeather ();
    PutMisc ();
    PutQ_And_Freq ();
}

PutQSO (void)
{
    /*  printf("VVV VVV\n") ; */
    PutFirstCallsign ();
    switch (Roll (6))
      {
      case 0:
	  PutForm0 ();
	  break;
      case 1:
	  PutForm1 ();
	  break;
      case 2:
	  PutForm2 ();
	  break;
      case 3:
	  PutForm3 ();
	  break;
      case 4:
	  PutForm4 ();
	  break;
      default:
	  PutForm5 ();
	  break;
      }
    printf ("+ %%\n");
    PutLastCallsign ();
    printf ("\n");
}
