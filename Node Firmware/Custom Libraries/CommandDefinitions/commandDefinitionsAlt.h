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
//Alternate commands for 0-9, a-z in ASCII for debugging with a keyboard
  #define bsmt_requestFWVer            65 //A
  #define bsmt_requestFurnaceStatus    66 //B
  #define bsmt_turnFurnaceOn           67 //C
  #define bsmt_turnFurnaceOff          68 //D
  #define bsmt_turnFanOn               69 //E
  #define bsmt_turnFurnaceAndFanOff    70 //F
  #define bsmt_requestTemp             71 //G
  #define bsmt_requestHumidity         72 //H
  #define bsmt_requestMoistureStatus   73 //I
  #define bsmt_requestCPUTemp          74 //J
  
  
//GARAGE COMMANDS
//Commands from the controller
//Alternate commands for 0-9, a-z in ASCII for debugging with a keyboard
  #define grge_requestFWVer            48 //0
  #define grge_requestDoorStatus       49 //1
  #define grge_requestTempZone1        50 //2
  #define grge_requestActivateDoor     51 //3
  #define grge_requestAutoCloseStatus  52 //4
  #define grge_requestDisableAutoClose 53 //5
  #define grge_requestEnableAutoClose  54 //6
  #define grge_requestActivate120V1    55 //7
  #define grge_requestDeactivate120V1  56 //8
  #define grge_requestCPUtemp          57 //9
  #define grge_requestTempZone2        97 //a
  #define grge_requestClearErrorFlag   98 //b
  #define grge_requestPowerSupplyV     99 //c
  
