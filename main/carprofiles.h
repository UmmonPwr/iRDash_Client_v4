////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////// Car profiles                                                           ////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#define NAMELENGTH      12
#define NUMOFGEARS      10   // maximum number of gears including reverse and neutral
// define car identification numbers
#define NUMOFCARS       12   // number of car profiles, maximum is 49
#define DEFAULTCAR      3    // car profile to show at startup
#define ID_Skippy       0    // Skip Barber
#define ID_FR20         1    // Formula Renault 2.0
#define ID_DF3          2    // Dallara Formula 3
#define ID_SFL          3    // Super Formula Lights
#define ID_CTS_V        4    // Cadillac CTS-V
#define ID_MX5_NC       5    // Mazda MX5 NC
#define ID_MX5_ND       6    // Mazda MX5 ND
#define ID_GR86         7    // Toyota GR86
#define ID_M2_CSR       8    // BMW M2 CS Racing
#define ID_M4_G82_GT4   9    // BMW G82 M4 GT4
#define ID_992_1_CUP    10   // Porsche 911 GT3 cup (992.1)
#define ID_992_2_CUP    11   // Porsche 911 GT3 cup (992.2)

// limits where different drawing color have to be used
typedef struct
{
  char CarName[NAMELENGTH];
  unsigned char ID;
  int Fuel;                   // value which below the warning color is used (value in liter * 10)
  int RPM;                    // max value of RPM gauge
  int WaterTemp;              // value in Celsius
  int SLI[NUMOFGEARS][8];     // RPM values for each SLI light
} SCarProfile;

// variables to manage screen layout
SCarProfile CarProfile[NUMOFCARS];     // store warning limits for each car

void UploadCarProfiles()
{
  /**************************/
  // Skip Barber
  /**************************/
  CarProfile[ID_Skippy].CarName[0] = 'S';
  CarProfile[ID_Skippy].CarName[1] = 'k';
  CarProfile[ID_Skippy].CarName[2] = 'i';
  CarProfile[ID_Skippy].CarName[3] = 'p';
  CarProfile[ID_Skippy].CarName[4] = 'p';
  CarProfile[ID_Skippy].CarName[5] = 'y';
  CarProfile[ID_Skippy].CarName[6] = 0;
  CarProfile[ID_Skippy].ID = ID_Skippy;

  CarProfile[ID_Skippy].Fuel = 30;
  CarProfile[ID_Skippy].WaterTemp = 90;
  CarProfile[ID_Skippy].RPM = 6600;

  for (int i = 0; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_Skippy].SLI[i][0] = 4700;
    CarProfile[ID_Skippy].SLI[i][1] = 4900;
    CarProfile[ID_Skippy].SLI[i][2] = 5100;
    CarProfile[ID_Skippy].SLI[i][3] = 5300;
    CarProfile[ID_Skippy].SLI[i][4] = 5500;
    CarProfile[ID_Skippy].SLI[i][5] = 5700;
    CarProfile[ID_Skippy].SLI[i][6] = 5900;
    CarProfile[ID_Skippy].SLI[i][7] = 6100;
  }
                               
  /**************************/
  // Cadillac CTS-V
  /**************************/
  CarProfile[ID_CTS_V].CarName[0] = 'C';
  CarProfile[ID_CTS_V].CarName[1] = 'T';
  CarProfile[ID_CTS_V].CarName[2] = 'S';
  CarProfile[ID_CTS_V].CarName[3] = '-';
  CarProfile[ID_CTS_V].CarName[4] = 'V';
  CarProfile[ID_CTS_V].CarName[5] = 0;
  CarProfile[ID_CTS_V].ID = ID_CTS_V;

  CarProfile[ID_CTS_V].Fuel = 50;
  CarProfile[ID_CTS_V].WaterTemp = 110;
  CarProfile[ID_CTS_V].RPM = 8000;

  for (int i = 0; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_CTS_V].SLI[i][0] = 6600;
    CarProfile[ID_CTS_V].SLI[i][1] = 6700;
    CarProfile[ID_CTS_V].SLI[i][2] = 6800;
    CarProfile[ID_CTS_V].SLI[i][3] = 6900;
    CarProfile[ID_CTS_V].SLI[i][4] = 7000;
    CarProfile[ID_CTS_V].SLI[i][5] = 7100;
    CarProfile[ID_CTS_V].SLI[i][6] = 7200;
    CarProfile[ID_CTS_V].SLI[i][7] = 7300;
  }
  
  /**************************/
  // Mazda MX-5 NC
  /**************************/
  CarProfile[ID_MX5_NC].CarName[0] = 'M';
  CarProfile[ID_MX5_NC].CarName[1] = 'X';
  CarProfile[ID_MX5_NC].CarName[2] = '5';
  CarProfile[ID_MX5_NC].CarName[3] = ' ';
  CarProfile[ID_MX5_NC].CarName[4] = 'N';
  CarProfile[ID_MX5_NC].CarName[5] = 'C';
  CarProfile[ID_MX5_NC].CarName[6] = 0;
  CarProfile[ID_MX5_NC].ID = ID_MX5_NC;

  CarProfile[ID_MX5_NC].Fuel = 40;
  CarProfile[ID_MX5_NC].WaterTemp = 100;
  CarProfile[ID_MX5_NC].RPM = 7300;

  for (int i = 0; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_MX5_NC].SLI[i][0] = 6000;
    CarProfile[ID_MX5_NC].SLI[i][1] = 6125;
    CarProfile[ID_MX5_NC].SLI[i][2] = 6250;
    CarProfile[ID_MX5_NC].SLI[i][3] = 6375;
    CarProfile[ID_MX5_NC].SLI[i][4] = 6500;
    CarProfile[ID_MX5_NC].SLI[i][5] = 6625;
    CarProfile[ID_MX5_NC].SLI[i][6] = 6750;
    CarProfile[ID_MX5_NC].SLI[i][7] = 6850;
  }

  /**************************/
  // Mazda MX-5 ND
  /**************************/
  CarProfile[ID_MX5_ND].CarName[0] = 'M';
  CarProfile[ID_MX5_ND].CarName[1] = 'X';
  CarProfile[ID_MX5_ND].CarName[2] = '5';
  CarProfile[ID_MX5_ND].CarName[3] = ' ';
  CarProfile[ID_MX5_ND].CarName[4] = 'N';
  CarProfile[ID_MX5_ND].CarName[5] = 'D';
  CarProfile[ID_MX5_ND].CarName[6] = 0;
  CarProfile[ID_MX5_ND].ID = ID_MX5_ND;

  CarProfile[ID_MX5_ND].Fuel = 40;
  CarProfile[ID_MX5_ND].WaterTemp = 100;
  CarProfile[ID_MX5_ND].RPM = 7500;

  for (int i = 0; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_MX5_ND].SLI[i][0] = 4600;
    CarProfile[ID_MX5_ND].SLI[i][1] = 5000;
    CarProfile[ID_MX5_ND].SLI[i][2] = 5400;
    CarProfile[ID_MX5_ND].SLI[i][3] = 5750;
    CarProfile[ID_MX5_ND].SLI[i][4] = 6100;
    CarProfile[ID_MX5_ND].SLI[i][5] = 6800;
    CarProfile[ID_MX5_ND].SLI[i][6] = 7200;
    CarProfile[ID_MX5_ND].SLI[i][7] = 7200;
  }

  /**************************/
  // Formula Renault 2.0
  /**************************/
  CarProfile[ID_FR20].CarName[0] = 'F';
  CarProfile[ID_FR20].CarName[1] = 'R';
  CarProfile[ID_FR20].CarName[2] = ' ';
  CarProfile[ID_FR20].CarName[3] = '2';
  CarProfile[ID_FR20].CarName[4] = '.';
  CarProfile[ID_FR20].CarName[5] = '0';
  CarProfile[ID_FR20].CarName[6] = 0;
  CarProfile[ID_FR20].ID = ID_FR20;

  CarProfile[ID_FR20].Fuel = 50;
  CarProfile[ID_FR20].WaterTemp = 90;
  CarProfile[ID_FR20].RPM = 7600;

  for (int i = 0; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_FR20].SLI[i][0] = 6600;
    CarProfile[ID_FR20].SLI[i][1] = 6700;
    CarProfile[ID_FR20].SLI[i][2] = 6800;
    CarProfile[ID_FR20].SLI[i][3] = 6900;
    CarProfile[ID_FR20].SLI[i][4] = 7000;
    CarProfile[ID_FR20].SLI[i][5] = 7100;
    CarProfile[ID_FR20].SLI[i][6] = 7200;
    CarProfile[ID_FR20].SLI[i][7] = 7300;
  }

  /**************************/
  // Dallara Formula 3
  /**************************/
  CarProfile[ID_DF3].CarName[0] = 'D';
  CarProfile[ID_DF3].CarName[1] = 'a';
  CarProfile[ID_DF3].CarName[2] = 'l';
  CarProfile[ID_DF3].CarName[3] = 'l';
  CarProfile[ID_DF3].CarName[4] = ' ';
  CarProfile[ID_DF3].CarName[5] = 'F';
  CarProfile[ID_DF3].CarName[6] = '3';
  CarProfile[ID_DF3].CarName[7] = 0;
  CarProfile[ID_DF3].ID = ID_DF3;

  CarProfile[ID_DF3].Fuel = 50;
  CarProfile[ID_DF3].WaterTemp = 110;
  CarProfile[ID_DF3].RPM = 7400;

  for (int i = 0; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_DF3].SLI[i][0] = 6450;
    CarProfile[ID_DF3].SLI[i][1] = 6550;
    CarProfile[ID_DF3].SLI[i][2] = 6650;
    CarProfile[ID_DF3].SLI[i][3] = 6750;
    CarProfile[ID_DF3].SLI[i][4] = 6850;
    CarProfile[ID_DF3].SLI[i][5] = 6950;
    CarProfile[ID_DF3].SLI[i][6] = 7050;
    CarProfile[ID_DF3].SLI[i][7] = 7150;
  }

  /**************************/
  // Porsche 992.1 GT3 cup
  /**************************/
  CarProfile[ID_992_1_CUP].CarName[0] = '9';
  CarProfile[ID_992_1_CUP].CarName[1] = '9';
  CarProfile[ID_992_1_CUP].CarName[2] = '2';
  CarProfile[ID_992_1_CUP].CarName[3] = '.';
  CarProfile[ID_992_1_CUP].CarName[4] = '1';
  CarProfile[ID_992_1_CUP].CarName[5] = ' ';
  CarProfile[ID_992_1_CUP].CarName[6] = 'c';
  CarProfile[ID_992_1_CUP].CarName[7] = 'u';
  CarProfile[ID_992_1_CUP].CarName[8] = 'p';
  CarProfile[ID_992_1_CUP].CarName[9] = 0;
  CarProfile[ID_992_1_CUP].ID = ID_992_1_CUP;

  CarProfile[ID_992_1_CUP].Fuel = 50;
  CarProfile[ID_992_1_CUP].WaterTemp = 110;
  CarProfile[ID_992_1_CUP].RPM = 8800;

  for (int i = 0; i<2; i++)
  // reverse, neutral
  {
    CarProfile[ID_992_1_CUP].SLI[i][0] = 7000; // 1st green
    CarProfile[ID_992_1_CUP].SLI[i][1] = 7200; // 2nd green
    CarProfile[ID_992_1_CUP].SLI[i][2] = 7400; // 3rd green
    CarProfile[ID_992_1_CUP].SLI[i][3] = 7600; // 1st yellow
    CarProfile[ID_992_1_CUP].SLI[i][4] = 7800; // 2nd yellow
    CarProfile[ID_992_1_CUP].SLI[i][5] = 8000; // 3rd yellow
    CarProfile[ID_992_1_CUP].SLI[i][6] = 8200; // 1st red
    CarProfile[ID_992_1_CUP].SLI[i][7] = 8400; // 2nd red
  } // all blue and blinking is not recorded

  // 1st gear
  CarProfile[ID_992_1_CUP].SLI[2][0] = 1500;
  CarProfile[ID_992_1_CUP].SLI[2][1] = 1500;
  CarProfile[ID_992_1_CUP].SLI[2][2] = 1500;
  CarProfile[ID_992_1_CUP].SLI[2][3] = 2000;
  CarProfile[ID_992_1_CUP].SLI[2][4] = 3200;
  CarProfile[ID_992_1_CUP].SLI[2][5] = 4350;
  CarProfile[ID_992_1_CUP].SLI[2][6] = 5540;
  CarProfile[ID_992_1_CUP].SLI[2][7] = 6720;

  // 2nd gear
  CarProfile[ID_992_1_CUP].SLI[3][0] = 2800;
  CarProfile[ID_992_1_CUP].SLI[3][1] = 3480;
  CarProfile[ID_992_1_CUP].SLI[3][2] = 4135;
  CarProfile[ID_992_1_CUP].SLI[3][3] = 4820;
  CarProfile[ID_992_1_CUP].SLI[3][4] = 5500;
  CarProfile[ID_992_1_CUP].SLI[3][5] = 6175;
  CarProfile[ID_992_1_CUP].SLI[3][6] = 6850;
  CarProfile[ID_992_1_CUP].SLI[3][7] = 7520;

  // 3rd gear
  CarProfile[ID_992_1_CUP].SLI[4][0] = 5200;
  CarProfile[ID_992_1_CUP].SLI[4][1] = 5600;
  CarProfile[ID_992_1_CUP].SLI[4][2] = 6000;
  CarProfile[ID_992_1_CUP].SLI[4][3] = 6400;
  CarProfile[ID_992_1_CUP].SLI[4][4] = 6800;
  CarProfile[ID_992_1_CUP].SLI[4][5] = 7200;
  CarProfile[ID_992_1_CUP].SLI[4][6] = 7600;
  CarProfile[ID_992_1_CUP].SLI[4][7] = 8000;

  // 4th gear
  CarProfile[ID_992_1_CUP].SLI[5][0] = 6700;
  CarProfile[ID_992_1_CUP].SLI[5][1] = 6925;
  CarProfile[ID_992_1_CUP].SLI[5][2] = 7150;
  CarProfile[ID_992_1_CUP].SLI[5][3] = 7375;
  CarProfile[ID_992_1_CUP].SLI[5][4] = 7600;
  CarProfile[ID_992_1_CUP].SLI[5][5] = 7825;
  CarProfile[ID_992_1_CUP].SLI[5][6] = 8050;
  CarProfile[ID_992_1_CUP].SLI[5][7] = 8275;

  // 5th gear
  CarProfile[ID_992_1_CUP].SLI[6][0] = 7600;
  CarProfile[ID_992_1_CUP].SLI[6][1] = 7725;
  CarProfile[ID_992_1_CUP].SLI[6][2] = 7850;
  CarProfile[ID_992_1_CUP].SLI[6][3] = 7975;
  CarProfile[ID_992_1_CUP].SLI[6][4] = 8100;
  CarProfile[ID_992_1_CUP].SLI[6][5] = 8225;
  CarProfile[ID_992_1_CUP].SLI[6][6] = 8350;
  CarProfile[ID_992_1_CUP].SLI[6][7] = 8475;

  // 6th gear and above
  for (int i = 7; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_992_1_CUP].SLI[i][0] = 8600;
    CarProfile[ID_992_1_CUP].SLI[i][1] = 8650;
    CarProfile[ID_992_1_CUP].SLI[i][2] = 8700;
    CarProfile[ID_992_1_CUP].SLI[i][3] = 8750;
    CarProfile[ID_992_1_CUP].SLI[i][4] = 8800;
    CarProfile[ID_992_1_CUP].SLI[i][5] = 8850;
    CarProfile[ID_992_1_CUP].SLI[i][6] = 8900;
    CarProfile[ID_992_1_CUP].SLI[i][7] = 8950;
  }

  /**************************/
  // Toyota GR86
  /**************************/
  CarProfile[ID_GR86].CarName[0] = 'G';
  CarProfile[ID_GR86].CarName[1] = 'R';
  CarProfile[ID_GR86].CarName[2] = '8';
  CarProfile[ID_GR86].CarName[3] = '6';
  CarProfile[ID_GR86].CarName[4] = 0;
  CarProfile[ID_GR86].ID = ID_GR86;
 
  CarProfile[ID_GR86].Fuel = 50;
  CarProfile[ID_GR86].WaterTemp = 95;
  CarProfile[ID_GR86].RPM = 7500;

  for (int i = 0; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_GR86].SLI[i][0] = 6000; // 1st light
    CarProfile[ID_GR86].SLI[i][1] = 6225; // added middle
    CarProfile[ID_GR86].SLI[i][2] = 6450; // 2nd light
    CarProfile[ID_GR86].SLI[i][3] = 6575; // added middle
    CarProfile[ID_GR86].SLI[i][4] = 6700; // 3rd light
    CarProfile[ID_GR86].SLI[i][5] = 6825; // added middle
    CarProfile[ID_GR86].SLI[i][6] = 6950; // 4th light
    CarProfile[ID_GR86].SLI[i][7] = 7250; // all blue
  }

  /**************************/
  // Super Formula Lights
  /**************************/
  CarProfile[ID_SFL].CarName[0] = 'S';
  CarProfile[ID_SFL].CarName[1] = 'F';
  CarProfile[ID_SFL].CarName[2] = 'L';
  CarProfile[ID_SFL].CarName[3] = 0;
  CarProfile[ID_SFL].ID = ID_SFL;

  CarProfile[ID_SFL].Fuel = 50;
  CarProfile[ID_SFL].WaterTemp = 110;
  CarProfile[ID_SFL].RPM = 7400;

  for (int i = 0; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_SFL].SLI[i][0] = 6200;
    CarProfile[ID_SFL].SLI[i][1] = 6300;
    CarProfile[ID_SFL].SLI[i][2] = 6400;
    CarProfile[ID_SFL].SLI[i][3] = 6500;
    CarProfile[ID_SFL].SLI[i][4] = 6600;
    CarProfile[ID_SFL].SLI[i][5] = 6700;
    CarProfile[ID_SFL].SLI[i][6] = 6800;
    CarProfile[ID_SFL].SLI[i][7] = 6900;
  }

  /**************************/
  // BMW M4 G82
  /**************************/
  CarProfile[ID_M4_G82_GT4].CarName[0] = 'M';
  CarProfile[ID_M4_G82_GT4].CarName[1] = '4';
  CarProfile[ID_M4_G82_GT4].CarName[2] = ' ';
  CarProfile[ID_M4_G82_GT4].CarName[3] = 'G';
  CarProfile[ID_M4_G82_GT4].CarName[4] = '8';
  CarProfile[ID_M4_G82_GT4].CarName[5] = '2';
  CarProfile[ID_M4_G82_GT4].CarName[6] = ' ';
  CarProfile[ID_M4_G82_GT4].CarName[7] = 'G';
  CarProfile[ID_M4_G82_GT4].CarName[8] = 'T';
  CarProfile[ID_M4_G82_GT4].CarName[9] = '4';
  CarProfile[ID_M4_G82_GT4].CarName[10] = 0;
  CarProfile[ID_M4_G82_GT4].ID = ID_M4_G82_GT4;

  CarProfile[ID_M4_G82_GT4].Fuel = 70;
  CarProfile[ID_M4_G82_GT4].WaterTemp = 95;
  CarProfile[ID_M4_G82_GT4].RPM = 7500;

  for (int i = 0; i<3; i++)
  // reverse, neutral, 1st
  {
    CarProfile[ID_M4_G82_GT4].SLI[i][0] = 5000; // 1st green
    CarProfile[ID_M4_G82_GT4].SLI[i][1] = 5000; // 1st green
    CarProfile[ID_M4_G82_GT4].SLI[i][2] = 5400; // 2nd green
    CarProfile[ID_M4_G82_GT4].SLI[i][3] = 5400; // 2nd green
    CarProfile[ID_M4_G82_GT4].SLI[i][4] = 5800; // 1st yellow
    CarProfile[ID_M4_G82_GT4].SLI[i][5] = 6200; // 2nd yellow
    CarProfile[ID_M4_G82_GT4].SLI[i][6] = 6600; // 1st red
    CarProfile[ID_M4_G82_GT4].SLI[i][7] = 7000; // all red and blinking
  }

  // 2nd gear
  CarProfile[ID_M4_G82_GT4].SLI[3][0] = 5350;
  CarProfile[ID_M4_G82_GT4].SLI[3][1] = 5350;
  CarProfile[ID_M4_G82_GT4].SLI[3][2] = 5675;
  CarProfile[ID_M4_G82_GT4].SLI[3][3] = 5675;
  CarProfile[ID_M4_G82_GT4].SLI[3][4] = 6010;
  CarProfile[ID_M4_G82_GT4].SLI[3][5] = 6340;
  CarProfile[ID_M4_G82_GT4].SLI[3][6] = 6670;
  CarProfile[ID_M4_G82_GT4].SLI[3][7] = 7000;

  // 3rd gear
  CarProfile[ID_M4_G82_GT4].SLI[4][0] = 6000;
  CarProfile[ID_M4_G82_GT4].SLI[4][1] = 6000;
  CarProfile[ID_M4_G82_GT4].SLI[4][2] = 6200;
  CarProfile[ID_M4_G82_GT4].SLI[4][3] = 6200;
  CarProfile[ID_M4_G82_GT4].SLI[4][4] = 6400;
  CarProfile[ID_M4_G82_GT4].SLI[4][5] = 6600;
  CarProfile[ID_M4_G82_GT4].SLI[4][6] = 6800;
  CarProfile[ID_M4_G82_GT4].SLI[4][7] = 7000;

  // 4th gear
  CarProfile[ID_M4_G82_GT4].SLI[5][0] = 6450;
  CarProfile[ID_M4_G82_GT4].SLI[5][1] = 6450;
  CarProfile[ID_M4_G82_GT4].SLI[5][2] = 6560;
  CarProfile[ID_M4_G82_GT4].SLI[5][3] = 6560;
  CarProfile[ID_M4_G82_GT4].SLI[5][4] = 6670;
  CarProfile[ID_M4_G82_GT4].SLI[5][5] = 6780;
  CarProfile[ID_M4_G82_GT4].SLI[5][6] = 6890;
  CarProfile[ID_M4_G82_GT4].SLI[5][7] = 7000;

  // 5th gear
  CarProfile[ID_M4_G82_GT4].SLI[6][0] = 6800;
  CarProfile[ID_M4_G82_GT4].SLI[6][1] = 6800;
  CarProfile[ID_M4_G82_GT4].SLI[6][2] = 6840;
  CarProfile[ID_M4_G82_GT4].SLI[6][3] = 6840;
  CarProfile[ID_M4_G82_GT4].SLI[6][4] = 6880;
  CarProfile[ID_M4_G82_GT4].SLI[6][5] = 6920;
  CarProfile[ID_M4_G82_GT4].SLI[6][6] = 6960;
  CarProfile[ID_M4_G82_GT4].SLI[6][7] = 7000;
 
  // 6th gear and above
  for (int i = 7; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_M4_G82_GT4].SLI[i][0] = 6900;
    CarProfile[ID_M4_G82_GT4].SLI[i][1] = 6900;
    CarProfile[ID_M4_G82_GT4].SLI[i][2] = 6920;
    CarProfile[ID_M4_G82_GT4].SLI[i][3] = 6920;
    CarProfile[ID_M4_G82_GT4].SLI[i][4] = 6940;
    CarProfile[ID_M4_G82_GT4].SLI[i][5] = 6960;
    CarProfile[ID_M4_G82_GT4].SLI[i][6] = 6980;
    CarProfile[ID_M4_G82_GT4].SLI[i][7] = 7000;
  }

  /**************************/
  // BMW M2 CSR
  /**************************/
  CarProfile[ID_M2_CSR].CarName[0] = 'M';
  CarProfile[ID_M2_CSR].CarName[1] = '2';
  CarProfile[ID_M2_CSR].CarName[2] = ' ';
  CarProfile[ID_M2_CSR].CarName[3] = 'C';
  CarProfile[ID_M2_CSR].CarName[4] = 'S';
  CarProfile[ID_M2_CSR].CarName[5] = 'R';
  CarProfile[ID_M2_CSR].CarName[6] = 0;
  CarProfile[ID_M2_CSR].ID = ID_M2_CSR;

  CarProfile[ID_M2_CSR].Fuel = 70;
  CarProfile[ID_M2_CSR].WaterTemp = 95;
  CarProfile[ID_M2_CSR].RPM = 7500;

  for (int i = 0; i<3; i++)
  // reverse, neutral, 1st gear
  {
    CarProfile[ID_M2_CSR].SLI[i][0] = 5970; // 1st green
    CarProfile[ID_M2_CSR].SLI[i][1] = 5970; // 1st green
    CarProfile[ID_M2_CSR].SLI[i][2] = 6240; // 2nd green
    CarProfile[ID_M2_CSR].SLI[i][3] = 6240; // 2nd green
    CarProfile[ID_M2_CSR].SLI[i][4] = 6505; // 1st yellow
    CarProfile[ID_M2_CSR].SLI[i][5] = 6775; // 2nd yellow
    CarProfile[ID_M2_CSR].SLI[i][6] = 7050; // 1st red
    CarProfile[ID_M2_CSR].SLI[i][7] = 7050; // 1st red
  }

  // 2nd gear
  CarProfile[ID_M2_CSR].SLI[3][0] = 5990;
  CarProfile[ID_M2_CSR].SLI[3][1] = 5990;
  CarProfile[ID_M2_CSR].SLI[3][2] = 6230;
  CarProfile[ID_M2_CSR].SLI[3][3] = 6230;
  CarProfile[ID_M2_CSR].SLI[3][4] = 6470;
  CarProfile[ID_M2_CSR].SLI[3][5] = 6710;
  CarProfile[ID_M2_CSR].SLI[3][6] = 6950;
  CarProfile[ID_M2_CSR].SLI[3][7] = 6950;

  // 3rd gear
  CarProfile[ID_M2_CSR].SLI[4][0] = 6190;
  CarProfile[ID_M2_CSR].SLI[4][1] = 6190;
  CarProfile[ID_M2_CSR].SLI[4][2] = 6330;
  CarProfile[ID_M2_CSR].SLI[4][3] = 6330;
  CarProfile[ID_M2_CSR].SLI[4][4] = 6465;
  CarProfile[ID_M2_CSR].SLI[4][5] = 6610;
  CarProfile[ID_M2_CSR].SLI[4][6] = 6750;
  CarProfile[ID_M2_CSR].SLI[4][7] = 6750;

  // 4th gear
  CarProfile[ID_M2_CSR].SLI[5][0] = 6445;
  CarProfile[ID_M2_CSR].SLI[5][1] = 6445;
  CarProfile[ID_M2_CSR].SLI[5][2] = 6500;
  CarProfile[ID_M2_CSR].SLI[5][3] = 6500;
  CarProfile[ID_M2_CSR].SLI[5][4] = 6550;
  CarProfile[ID_M2_CSR].SLI[5][5] = 6600;
  CarProfile[ID_M2_CSR].SLI[5][6] = 6650;
  CarProfile[ID_M2_CSR].SLI[5][7] = 6650;

  // 5th gear and above
  for (int i = 6; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_M2_CSR].SLI[i][0] = 6367;
    CarProfile[ID_M2_CSR].SLI[i][1] = 6367;
    CarProfile[ID_M2_CSR].SLI[i][2] = 6387;
    CarProfile[ID_M2_CSR].SLI[i][3] = 6387;
    CarProfile[ID_M2_CSR].SLI[i][4] = 6407;
    CarProfile[ID_M2_CSR].SLI[i][5] = 6427;
    CarProfile[ID_M2_CSR].SLI[i][6] = 6447;
    CarProfile[ID_M2_CSR].SLI[i][7] = 6447;
  }

  /**************************/
  // Porsche 992.2 GT3 cup
  /**************************/
  CarProfile[ID_992_2_CUP].CarName[0] = '9';
  CarProfile[ID_992_2_CUP].CarName[1] = '9';
  CarProfile[ID_992_2_CUP].CarName[2] = '2';
  CarProfile[ID_992_2_CUP].CarName[3] = '.';
  CarProfile[ID_992_2_CUP].CarName[4] = '2';
  CarProfile[ID_992_2_CUP].CarName[5] = ' ';
  CarProfile[ID_992_2_CUP].CarName[6] = 'c';
  CarProfile[ID_992_2_CUP].CarName[7] = 'u';
  CarProfile[ID_992_2_CUP].CarName[8] = 'p';
  CarProfile[ID_992_2_CUP].CarName[9] = 0;
  CarProfile[ID_992_2_CUP].ID = ID_992_2_CUP;

  CarProfile[ID_992_2_CUP].Fuel = 50;
  CarProfile[ID_992_2_CUP].WaterTemp = 110;
  CarProfile[ID_992_2_CUP].RPM = 8800;

  for (int i = 0; i<2; i++)
  // reverse, neutral
  {
    CarProfile[ID_992_2_CUP].SLI[i][0] = 7000; // 1st green
    CarProfile[ID_992_2_CUP].SLI[i][1] = 7200; // 2nd green
    CarProfile[ID_992_2_CUP].SLI[i][2] = 7400; // 3rd green
    CarProfile[ID_992_2_CUP].SLI[i][3] = 7600; // 1st yellow
    CarProfile[ID_992_2_CUP].SLI[i][4] = 7800; // 2nd yellow
    CarProfile[ID_992_2_CUP].SLI[i][5] = 8000; // 3rd yellow
    CarProfile[ID_992_2_CUP].SLI[i][6] = 8200; // 1st red
    CarProfile[ID_992_2_CUP].SLI[i][7] = 8400; // 2nd red
  } // all blue and blinking is not recorded

  // 1st gear
  CarProfile[ID_992_2_CUP].SLI[2][0] = 1500;
  CarProfile[ID_992_2_CUP].SLI[2][1] = 1500;
  CarProfile[ID_992_2_CUP].SLI[2][2] = 1500;
  CarProfile[ID_992_2_CUP].SLI[2][3] = 2000;
  CarProfile[ID_992_2_CUP].SLI[2][4] = 3200;
  CarProfile[ID_992_2_CUP].SLI[2][5] = 4350;
  CarProfile[ID_992_2_CUP].SLI[2][6] = 5540;
  CarProfile[ID_992_2_CUP].SLI[2][7] = 6720;

  // 2nd gear
  CarProfile[ID_992_2_CUP].SLI[3][0] = 2800;
  CarProfile[ID_992_2_CUP].SLI[3][1] = 3480;
  CarProfile[ID_992_2_CUP].SLI[3][2] = 4135;
  CarProfile[ID_992_2_CUP].SLI[3][3] = 4820;
  CarProfile[ID_992_2_CUP].SLI[3][4] = 5500;
  CarProfile[ID_992_2_CUP].SLI[3][5] = 6175;
  CarProfile[ID_992_2_CUP].SLI[3][6] = 6850;
  CarProfile[ID_992_2_CUP].SLI[3][7] = 7520;

  // 3rd gear
  CarProfile[ID_992_2_CUP].SLI[4][0] = 5200;
  CarProfile[ID_992_2_CUP].SLI[4][1] = 5600;
  CarProfile[ID_992_2_CUP].SLI[4][2] = 6000;
  CarProfile[ID_992_2_CUP].SLI[4][3] = 6400;
  CarProfile[ID_992_2_CUP].SLI[4][4] = 6800;
  CarProfile[ID_992_2_CUP].SLI[4][5] = 7200;
  CarProfile[ID_992_2_CUP].SLI[4][6] = 7600;
  CarProfile[ID_992_2_CUP].SLI[4][7] = 8000;

  // 4th gear
  CarProfile[ID_992_2_CUP].SLI[5][0] = 6700;
  CarProfile[ID_992_2_CUP].SLI[5][1] = 6925;
  CarProfile[ID_992_2_CUP].SLI[5][2] = 7150;
  CarProfile[ID_992_2_CUP].SLI[5][3] = 7375;
  CarProfile[ID_992_2_CUP].SLI[5][4] = 7600;
  CarProfile[ID_992_2_CUP].SLI[5][5] = 7825;
  CarProfile[ID_992_2_CUP].SLI[5][6] = 8050;
  CarProfile[ID_992_2_CUP].SLI[5][7] = 8275;

  // 5th gear
  CarProfile[ID_992_2_CUP].SLI[6][0] = 7600;
  CarProfile[ID_992_2_CUP].SLI[6][1] = 7725;
  CarProfile[ID_992_2_CUP].SLI[6][2] = 7850;
  CarProfile[ID_992_2_CUP].SLI[6][3] = 7975;
  CarProfile[ID_992_2_CUP].SLI[6][4] = 8100;
  CarProfile[ID_992_2_CUP].SLI[6][5] = 8225;
  CarProfile[ID_992_2_CUP].SLI[6][6] = 8350;
  CarProfile[ID_992_2_CUP].SLI[6][7] = 8475;

  // 6th gear and above
  for (int i = 7; i<NUMOFGEARS; i++)
  {
    CarProfile[ID_992_2_CUP].SLI[i][0] = 8600;
    CarProfile[ID_992_2_CUP].SLI[i][1] = 8650;
    CarProfile[ID_992_2_CUP].SLI[i][2] = 8700;
    CarProfile[ID_992_2_CUP].SLI[i][3] = 8750;
    CarProfile[ID_992_2_CUP].SLI[i][4] = 8800;
    CarProfile[ID_992_2_CUP].SLI[i][5] = 8850;
    CarProfile[ID_992_2_CUP].SLI[i][6] = 8900;
    CarProfile[ID_992_2_CUP].SLI[i][7] = 8950;
  }
}