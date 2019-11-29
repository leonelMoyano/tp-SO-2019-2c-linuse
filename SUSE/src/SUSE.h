/*
 * suseServer.h
 *
 *  Created on: 1 nov. 2019
 *      Author: utnso
 */

#ifndef SUSESERVER_H_
#define SUSESERVER_H_

#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<commons/log.h>
#include<commons/config.h>
#include "suseDefs.h"

t_log* logger;
t_config* configServer;

void iniciar_logger	(void);

#endif /* SUSESERVER_H_ */
