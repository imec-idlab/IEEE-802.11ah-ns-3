/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2009 MIRKO BANCHI
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as 
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

/*
beacon interval=0.1s
slotformat  rawslotcount slotnum
1             829              1
1             412              2 
1             204              4
 */


/*
Output current RawConfig information into file RawConfig.txt
Add current RawConfig information into file RawConfiglog.txt
 */



#include "ns3/core-module.h"
#include "ns3/network-module.h"
#include "ns3/applications-module.h"
#include "ns3/wifi-module.h"
#include "ns3/mobility-module.h"
#include "ns3/ipv4-global-routing-helper.h"
#include "ns3/internet-module.h"
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <ctime>
#include <fstream>
#include <iostream>



using namespace std;
using namespace ns3;

int main (int argc, char *argv[])
{
	uint32_t nsta;
	double throughput;
	bool random;
	CommandLine cmd;
	cmd.AddValue ("nsta", "number of stations", nsta);
	cmd.AddValue ("throughput", "number of RAW groups", throughput);
	cmd.AddValue ("random", "number of slot per Raw", random);
	cmd.Parse (argc,argv);

	std::ostringstream pathss;
	pathss << "./OptimalRawGroup/traffic/data-" << nsta << "-" << throughput << ".txt";
	std::string filepath = pathss.str();

	ofstream newfile;
	newfile.open (filepath, ios::out | ios::trunc);

	for (int i = 1; i <= nsta; i++)
	{
		newfile << i << "\t";
		if (random)
		{
			Ptr<UniformRandomVariable> m_rv = CreateObject<UniformRandomVariable>();
			double randval =  m_rv->GetValue(0, throughput);
			newfile << randval << "\n";
		}
		else
			newfile << throughput << "\n";
	}

	newfile.close();

	std::cout << "TrafficPath=" << pathss.str() << std::endl;
	return 0;
}












