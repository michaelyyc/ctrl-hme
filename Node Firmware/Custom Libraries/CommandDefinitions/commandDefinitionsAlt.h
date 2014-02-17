//CONTROLLER COMMANDS
  #define ctrl_requestFWVer              'K' //K
  #define ctrl_requestTemp               'L' //L
  #define ctrl_enableFurnace             'M' //M
  #define ctrl_disableFurnace            'N' //N
  #define ctrl_increaseTempSetPoint      'O' //O
  #define ctrl_decreaseTempSetPoint      'P' //P
  #define ctrl_listCommands              '?' // ?
  #define ctrl_logOff                    'x' // x
  #define ctrl_statusReport				 'S' //S
  #define ctrl_enableBlockHeater	     'Q' //Q
  #define ctrl_disableBlockHeater   	 'R' //R

//BASEMENT COMMANDS
//Commands from the controller
//Alternate commands for 0-9, a-z in ASCII for debugging with a keyboard
  #define bsmt_requestFWVer            'A' //A
  #define bsmt_requestFurnaceStatus    'B' //B
  #define bsmt_turnFurnaceOn           'C' //C
  #define bsmt_turnFurnaceOff          'D' //D
  #define bsmt_turnFanOn               'E' //E
  #define bsmt_turnFurnaceAndFanOff    'F' //F
  #define bsmt_requestTemp             'G' //G
  #define bsmt_requestHumidity         'H' //H
  #define bsmt_requestMoistureStatus   'I' //I
  #define bsmt_requestCPUTemp          'J' //J
  
  
//GARAGE COMMANDS
//Commands from the controller
//Alternate commands for 0-9, a-z in ASCII for debugging with a keyboard
  #define grge_requestFWVer            '0' //0
  #define grge_requestDoorStatus       '1' //1
  #define grge_requestTempZone1        '2' //2
  #define grge_requestActivateDoor     '3' //3
  #define grge_requestAutoCloseStatus  '4' //4
  #define grge_requestDisableAutoClose '5' //5
  #define grge_requestEnableAutoClose  '6' //6
  #define grge_requestActivate120V1    '7' //7
  #define grge_requestDeactivate120V1  '8' //8
  #define grge_requestCPUtemp          '9' //9
  #define grge_requestTempZone2        'a' //a
  #define grge_requestClearErrorFlag   'b' //b
  #define grge_requestPowerSupplyV     'c' //c
  
  
  //BEDROOM COMMANDS
  //Commands from the controller
  //Alternat commands for 0-9, a-Z for ASCII input from keypard
  
  #define bdrm_requestFWVer				'i'
  #define bdrm_requestTemp				'j'
  #define bdrm_increaseSetPoint			'k'
  #define bdrm_decreaseSetPoint			'l'
  #define bdrm_maintainSetPoint 		'm'
  #define bdrm_dontMaintainSetPoint		'n'
  #define bdrm_requestSetPoint			'o'