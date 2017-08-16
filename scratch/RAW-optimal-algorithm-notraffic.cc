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

void RAWGroupping (uint16_t Numsta, uint16_t NGroups, uint16_t NumSlot, uint16_t trial, string RAWConfigPath, string RAWConfiglogPath)
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
    uint16_t page = 0;
    uint32_t aid_start = 0;
    uint32_t aid_end = 0;
    
    NRawSta = Numsta;
    NRawGroups = NGroups;
    SlotNum = NumSlot;
    //100000 = beacon interval
    SlotDurationCount = (102400/(SlotNum) - 500)/120;
    

    ofstream newfile;
    string filepath = RAWConfigPath;
    newfile.open (filepath, ios::out | ios::trunc);
    newfile << NGroups << "\n";
    newfile.close();
    
    string logfilepath = RAWConfiglogPath;
    newfile.open (logfilepath, ios::out | ios::app);
    newfile << trial << "\t" << NGroups << "\n";
    newfile.close();
    
    std::cout << "filepath=" << filepath << "\n";

    

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
       newfile.open (filepath, ios::out | ios::app);
       newfile << RawControl << "\t" << SlotCrossBoundary << "\t" << SlotFormat << "\t" << SlotDurationCount << "\t" << SlotNum << "\t" << page << "\t" << aid_start << "\t" << aid_end << "\t" << "\n";
       newfile.close();
            
       newfile.open (logfilepath, ios::out | ios::app);
       newfile << RawControl << "\t" << SlotCrossBoundary << "\t" << SlotFormat << "\t" << SlotDurationCount << "\t" << SlotNum << "\t" << page << "\t" << aid_start << "\t" << aid_end << "\t" << "\n";
       newfile.close();
      
      aid_start = aid_end + 1;
    }
}




int main (int argc, char *argv[])
{
  uint16_t NRawSta;
  uint16_t NGroup;
  uint16_t trial=0;
  uint16_t NumSlot;
  string RAWConfigPath;
  string RAWConfiglogPath;
    
    CommandLine cmd;
    cmd.AddValue ("NRawSta", "number of stations supporting RAW", NRawSta);
    cmd.AddValue ("NGroup", "number of RAW groups", NGroup);
    cmd.AddValue ("NumSlot", "number of slot per Raw", NumSlot);
    cmd.AddValue ("trial", "times that RAW group has been balanced", trial);
    cmd.AddValue ("RAWConfigPath", "RAW Config file Path", RAWConfigPath);
    cmd.AddValue ("RAWConfiglogPath", "RAW Config file Path", RAWConfiglogPath);

    cmd.Parse (argc,argv);
    
    std::cout << "NRawSta=" << NRawSta <<  " NGroup=" << NGroup << " trial=" << trial << "\n";
    
    void RAWGroupping (uint16_t, uint16_t, uint16_t, uint16_t, string, string);
    


 
    
  RAWGroupping (NRawSta, NGroup, NumSlot, trial, RAWConfigPath, RAWConfiglogPath);
    
  return 0;
}












