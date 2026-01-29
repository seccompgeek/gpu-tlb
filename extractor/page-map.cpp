#include "page-map.h"

/*******************************************************************************
 *
 ******************************************************************************/
PageMap::PageMap(MemDump &dump, std::uint64_t addr, TransType type) : 
    Trans(dump, addr, type)
{ }

/*******************************************************************************
 *
 ******************************************************************************/
PageMap::~PageMap()
{ 
  for (const auto &ent : mPageMapEnts)
    delete ent.second;
}

/*******************************************************************************
 *
 ******************************************************************************/
bool 
PageMap::constructTrans()
{
  // PD4 has 2 entries, PD3 has 4 entries, PD2 and PD1 have 512 entries
  int entNum = mTransType == PD4 ? 2 : 
               mTransType == PD3 ? 4 : 512;
  int entSize = 8;
  
  for (int i = 0; i < entNum; ++i) {
    std::uint64_t offset = mPhyAddr + i * entSize;
    
    // Read the full 64-bit entry
    std::uint64_t entry = 0;
    for (int j = 0; j < 8; ++j) {
      std::uint8_t byteVal = mMemDump.getByte(offset + j);
      entry |= ((std::uint64_t)byteVal) << (j * 8);
    }

    if(entry != 0)
      std::cout << "Entry " << i << ": 0x" << std::hex << entry << std::dec << std::endl;
    
    // Parse as NV_MMU_VER3 format
    // bit[0]=IS_PTE (0=PDE), bits[2:1]=APERTURE, bits[51:12]=ADDRESS
    bool is_pte = entry & 0x01;
    std::uint8_t aperture = (entry >> 1) & 0x03;
    
    // Extract address from bits [51:12]
    std::uint64_t addr = (entry >> 12) & 0xFFFFFFFFFFFULL;  // 40 bits
    std::uint64_t nPhyAddr = addr << 12;
    
    // Skip if it's a PTE (should be PDE), or aperture is invalid, or no address
    if (is_pte || aperture == 0)
      continue;
    
    // construct the next-level trans
    TransType nTransType = mTransType == PD4 ? PD3 :
                           mTransType == PD3 ? PD2 : 
                           mTransType == PD2 ? PD1 : PD0;
    
    Trans *next = nullptr;
    if (nTransType == PD0)
      next = new PageDir(mMemDump, nPhyAddr, nTransType);
    else
      next = new PageMap(mMemDump, nPhyAddr, nTransType);
    bool ok = next->constructTrans();
    if (!ok) {
      delete next;
      continue;
    }
    
    mPageMapEnts[i] = next;
  }
  
  if (mPageMapEnts.empty())
    return false;
  else
    return true;
}

/*******************************************************************************
 *
 ******************************************************************************/
void 
PageMap::printTrans(std::uint64_t virtAddr)
{
  if (mTransType == PD4)
    std::cout << "PD4@0x";
  else if (mTransType == PD3)
    std::cout << "\t" << std::dec << std::setw(3) << std::setfill(' ')
              << (virtAddr >> 56 & 0x001) << "-->PD3@0x";
  else if (mTransType == PD2)
    std::cout << "\t\t" << std::dec << std::setw(3) << std::setfill(' ')
              << (virtAddr >> 47 & 0x003) << "-->PD2@0x";
  else
    std::cout << "\t\t\t" << std::dec << std::setw(3) << std::setfill(' ')
              << (virtAddr >> 38 & 0x1FF) << "-->PD1@0x";
  std::cout << std::hex << std::setw(10) << std::setfill('0') 
            << mPhyAddr << std::endl;
  
  for (const auto &ent : mPageMapEnts) {
    int shAmt = mTransType == PD4 ? 56 :
                mTransType == PD3 ? 47 : 
                mTransType == PD2 ? 38 : 29;
    std::uint64_t virtAddrNew = virtAddr | (std::uint64_t)ent.first << shAmt;
    ent.second->printTrans(virtAddrNew);
  }
}


