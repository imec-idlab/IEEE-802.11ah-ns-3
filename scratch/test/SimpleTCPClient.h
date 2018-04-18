/*
 * StatisticsTCPClient.h
 *
 *  Created on: Jun 30, 2016
 *      Author: dwight
 */

#ifndef SCRATCH_AHSIMULATION_SIMPLETCPCLIENT_H_
#define SCRATCH_AHSIMULATION_SIMPLETCPCLIENT_H_

int stat_connect(const char* hostname, const char* port);
bool stat_send(int sockfd,const char* buf);
void stat_close(int sockfd);

#endif /* SCRATCH_AHSIMULATION_SIMPLETCPCLIENT_H_ */
