/*************************************************************************************************/
/*!
 *  \file
 *
 *  \brief      App SPI handler.
 *
 *  Copyright (c) 2015-2018 Arm Ltd. All Rights Reserved.
 *
 *  Copyright (c) 2019 Packetcraft, Inc.
 * 
 *  Portions Copyright (c) 2023 Analog Devices, Inc.
 *  
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *  
 *      http://www.apache.org/licenses/LICENSE-2.0
 *  
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */
/*************************************************************************************************/

#ifndef APP_SPI_H
#define APP_SPI_H

/*! \addtogroup APP_FRAMEWORK_UI_API
 *  \{ */

/**************************************************************************************************
  Function Prototypes
**************************************************************************************************/

/** \name APP SPI Functions
 * SPI interface to the application.
 */
/**@{*/

/*************************************************************************************************/
/*!
 *  \brief  Initialize SPI.
 *
 *  \return None.
 */
/*************************************************************************************************/
void AppSpiInit(void);

/**@}*/

/*! \} */    /*! APP_FRAMEWORK_UI_API */

#endif /* APP_SPI_H */
