//CONTROLLER COMMANDS
  	#define ctrl_requestFWVer              75 //K
  	#define ctrl_requestTemp               76 //L
 	#define ctrl_enableFurnace             77 //M
  	#define ctrl_disableFurnace            78 //N
  	#define ctrl_increaseTempSetPoint      79 //O
  	#define ctrl_decreaseTempSetPoint      80 //P
  	#define ctrl_listCommands              63 // ?
  	#define ctrl_logOff                   120 // x

//BASEMENT COMMANDS
//Commands from the controller

  #define bsmt_requestFWVer            32
  #define bsmt_requestFurnaceStatus    33
  #define bsmt_turnFurnaceOn           34
  #define bsmt_turnFurnaceOff          35
  #define bsmt_turnFanOn               36
  #define bsmt_turnFurnaceAndFanOff    37
  #define bsmt_requestTemp             38
  #define bsmt_requestHumidity         39
  #define bsmt_requestMoistureStatus   40
  #define bsmt_requestCPUTemp          43
 
  
//GARAGE COMMANDS
//Commands from the controller

  #define grge_requestFWVer            0
  #define grge_requestDoorStatus       1
  #define grge_requestTempZone1        2
  #define grge_requestActivateDoor     3
  #define grge_requestAutoCloseStatus  4
  #define grge_requestDisableAutoClose 5
  #define grge_requestEnableAutoClose  6
  #define grge_requestActivate120V1      7
  #define grge_requestDeactivate120V1  8
  #define grge_requestCPUtemp          9
  #define grge_requestTempZone2       10
  #define grge_requestClearErrorFlag  11
  #define grge_requestPowerSupplyV    12
