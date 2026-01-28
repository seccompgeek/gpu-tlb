#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include "mem-dump.h"
#include "trans.h"
#include "page-map.h"
#include "page-dir.h"
#include "page-tab.h"
#include "page.h"

/*******************************************************************************
 *
 ******************************************************************************/
int 
main(int argc, char *argv[])
{
  if (argc < 2 || argc > 3) {
    std::cout << "Usage: " << argv[0] << " <dump> [--blackwell]\n";
    std::cout << "  --blackwell: Use PD4 as top level for Blackwell GPUs\n";
    return -1;
  }
  
  // Check if Blackwell mode is enabled
  bool blackwell = false;
  if (argc == 3 && std::string(argv[2]) == "--blackwell") {
    blackwell = true;
    std::cout << "Blackwell mode: Starting from PD4\n";
  }
  
  MemDump dump(argv[1]);
  std::uint64_t chunkNum = dump.getChunkNum();
  
  // Use PD4 for Blackwell, PD3 for Hopper and earlier
  TransType topLevel = blackwell ? PD4 : PD3;
  
  std::vector<Trans *> pageMaps;
  for (std::uint64_t i = 0; i < chunkNum; ++i) {
    std::uint64_t phyAddr = i * CHUNK_SIZE;
    Trans *topPtr = new PageMap(dump, phyAddr, topLevel);
    bool ok = topPtr->constructTrans();
    if (!ok)
      delete topPtr;
    else
      pageMaps.push_back(topPtr);
  }
  
  for (auto topPtr : pageMaps)
    topPtr->printTrans(0);
}

