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
#include <sys/stat.h>
#include "ns3/rps.h"
#include <utility> // std::pair
#include <map>
#include <iostream>



using namespace std;
using namespace ns3;

void RAWGroupping (uint16_t Numsta, uint16_t NGroups, uint16_t NumSlot, uint16_t trial, string RAWConfigPath, uint32_t beaconinterval, uint32_t pageSliceCount, uint32_t pageSliceLen)
{
    uint16_t NRawSta;
    uint16_t NRawGroups;
    uint16_t NstaPerRaw;
    uint16_t NstaPerSlot;


    uint16_t RawControl = 0;
    uint16_t SlotCrossBoundary = 1;
    uint16_t SlotFormat = 1;
    uint16_t SlotDurationCount = 829;
    uint16_t SlotNum = 1;
    uint16_t pageIndex = 0;
    uint32_t aid_start = 0;
    uint32_t aid_end = 0;
    
    NRawSta = Numsta;
    NRawGroups = NGroups;
    SlotNum = NumSlot;
    SlotDurationCount = (beaconinterval/NGroups/(SlotNum) - 500)/120;
    
	if (Numsta < pageSliceLen * 64 && pageSliceCount != 0)
	{
		std::cerr << "All stations can fit to a single page slice. Too many page slices relative to the number of stations. Set PageSliceCount=0 PageSliceLength=1 or reduce Page Slice Length." << std::endl;
		NS_ASSERT (pageSliceCount == 0);
	}

    ofstream newfile;
    string filepath = RAWConfigPath;
    newfile.open (filepath, ios::out | ios::trunc);
    if (pageSliceCount == 0)
    {
     	newfile << 1 << "\n" << 1 << "\n";
     	//newfile.close();
     	//grouping
     	aid_start = 1;

     	NstaPerRaw = NRawSta/NRawGroups;
     	NstaPerSlot = NstaPerRaw/SlotNum;

     	if (SlotNum < 8)
     	{
     		SlotFormat = 1;
     	}
     	else
     	{
     		SlotFormat = 0;
     	}

     	for (uint16_t groupID = 1; groupID <= NRawGroups; groupID++)
     	{
     		aid_end = aid_start + NstaPerRaw - 1;
     		NS_ASSERT (pageIndex * 2048 < aid_end && aid_end < (pageIndex + 1) * 2048);
     		//newfile.open (filepath, ios::out | ios::app);
     		newfile << RawControl << "\t" << SlotCrossBoundary << "\t" << SlotFormat << "\t" << SlotDurationCount << "\t" << SlotNum << "\t" << pageIndex << "\t" << aid_start << "\t" << aid_end << "\t" << "\n";
     		//newfile.close();

     		aid_start = aid_end + 1;
     	}
    }
    else
    {
    	//newfile.open (filepath, ios::out | ios::trunc);
    	newfile << pageSliceCount << "\n";
    	//newfile.close();
    	for (int timID = 0; timID < pageSliceCount; timID++)
    	{
        	std::cout << "tim num=" << timID << std::endl;
    		newfile << NGroups << "\n";

    		uint32_t endAidInTim;
    		if (timID + 1 < pageSliceCount)
    		{
    			endAidInTim = (timID + 1) * pageSliceLen * 64 - 1;
    		}
    		else
    		{
    			endAidInTim = NRawSta;//(pageIndex + 1) * 2048 - 1;
    		}

    		std::cout << "NRawSTa=" << NRawSta << " endAidInTim=" << endAidInTim ;
    		if (NRawSta > endAidInTim || timID + 1 != pageSliceCount)
    		{
    			NstaPerRaw = pageSliceLen * 64 / NRawGroups;
    			if (timID == 0)
    				NRawSta -= pageSliceLen * 64 - 1;
    			else
    				NRawSta -= pageSliceLen * 64;
    		}
    		else
    		{
    			NstaPerRaw = NRawSta / NRawGroups;
    			//NRawSta = 0;
    		}
    		std::cout << " NstaPerRaw=" << NstaPerRaw << "NrawSTa-novo = " << NRawSta << std::endl;
    		//NstaPerSlot = NstaPerRaw/SlotNum;

    		//NstaPerRaw = (endAidInTim - aid_start + 1) / NRawGroups;

    		if (SlotNum < 8)
    		{
    			SlotFormat = 1;
    		}
    		else
    		{
    			SlotFormat = 0;
    		}

    		for (uint16_t groupID = 1; groupID <= NRawGroups; groupID++)
    		{
    			aid_end = aid_start + NstaPerRaw - 1;
    			std::cout << "RAWg=" << groupID << " start=" << aid_start << " end=" << aid_end <<std::endl;

    			NS_ASSERT (aid_end <= Numsta);
    			//newfile.open (filepath, ios::out | ios::app);
    			if (groupID == 1 && timID == 0) aid_start = 1;
    			newfile << RawControl << "\t" << SlotCrossBoundary << "\t" << SlotFormat << "\t" << SlotDurationCount << "\t" << SlotNum << "\t" << pageIndex << "\t" << aid_start << "\t" << aid_end << "\t" << "\n";
    			//newfile.close();

    			aid_start = aid_end + 1;
    		}
    		//newfile.open (filepath, ios::out | ios::app);
    		//newfile << "\n";
    		//newfile.close();
    	}
    }
    newfile.close();
}




int main (int argc, char *argv[])
{
  uint16_t NRawSta;
  uint16_t NGroup;
  uint16_t trial=0;
  uint16_t NumSlot;
  uint32_t beaconinterval;
  string RAWConfigPath;
  uint32_t pageSliceLen, pageSliceCount;
    CommandLine cmd;
    cmd.AddValue ("NRawSta", "number of stations supporting RAW", NRawSta);
    cmd.AddValue ("NGroup", "number of RAW groups", NGroup);
    cmd.AddValue ("NumSlot", "number of slot per Raw", NumSlot);
    cmd.AddValue ("beaconinterval", "beaconinterval (us)", beaconinterval);
    cmd.AddValue ("trial", "times that RAW group has been balanced", trial);
    cmd.AddValue ("RAWConfigPath", "RAW Config file Path", RAWConfigPath);
    cmd.AddValue ("pageSliceCount", "RAW Config file Path", pageSliceCount);
    cmd.AddValue ("pageSliceLen", "RAW Config file Path", pageSliceLen);

    cmd.Parse (argc,argv);
    
    std::cout << "NRawSta=" << NRawSta <<  " NGroup=" << NGroup << " beaconinterval=" << beaconinterval << "RAWConfigPath=" << RAWConfigPath << "\n";
    
    void RAWGroupping (uint16_t, uint16_t, uint16_t, uint16_t, string, uint32_t, uint32_t, uint32_t);
    


 
    
  RAWGroupping (NRawSta, NGroup, NumSlot, trial, RAWConfigPath, beaconinterval, pageSliceCount, pageSliceLen);
    
  return 0;
}












