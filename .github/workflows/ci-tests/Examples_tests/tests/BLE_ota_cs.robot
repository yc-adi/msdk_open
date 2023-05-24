*** settings ***
Library    String
Library    ../resources/serialPortReader.py
Suite Setup        Open Ports    ${SERIAL_PORT_1}    ${SERIAL_PORT_2}
Suite Teardown     Close Ports 

*** Variables ***
${SERIAL_PORT_1}  /dev/ttyUSB0
${SERIAL_PORT_2}  /dev/ttyUSB1

*** test cases ***
Original Firmware Test
    Log To Console        >>> DUT
    [Timeout]    30s
    Sleep     5s
    Expect And Timeout    btn 2 m\n    FW_VERSION: 1.0     5     ${SERIAL_PORT_2}

File Discovery Test
    Log To Console        >>> Main Device
    [Timeout]    30s
    sleep    1s
    Expect And Timeout    btn 2 s\n    >>> File discovery complete <<<    5    ${SERIAL_PORT_1}

File Transfer Test
    Log To Console        >>> Main Device
    [Timeout]    60s
    sleep    1s
    Expect And Timeout    btn 2 m\n    >>> File transfer complete    20    ${SERIAL_PORT_1}

File Verification Test
    Log To Console        >>> Main Device
    [Timeout]             30S
    sleep                 1s
    Expect And Timeout    btn 2 l\n    >>> Verify complete status: 0 <<<    2    ${SERIAL_PORT_1}
   
Peer Device Reset Test
    Log To Console        >>> Main Device
    [Timeout]             40s
    sleep                 1s
    Expect And Timeout    btn 2 x\n    >>> Scanning started <<<    30    ${SERIAL_PORT_1}

Firmware Update Verification Test
    Log To Console        >>> DUT
    [Timeout]             60s
    Read All              FW_VERSION: 2.0     40     ${SERIAL_PORT_2}


Firmware Reconnect Succesful Test
    Log To Console        >>> Main Device
    [Timeout]             60s
    sleep                 10s
    Expect And Timeout    btn 2 s\n    >>> File discovery complete <<<    15    ${SERIAL_PORT_1}
    

    

