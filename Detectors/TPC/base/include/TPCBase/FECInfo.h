#ifndef AliceO2_TPC_FECInfo_H
#define AliceO2_TPC_FECInfo_H

namespace AliceO2 {
namespace TPC {

class FECInfo {
  public:
    FECInfo() {}
    FECInfo(unsigned char index,
            unsigned char connector,
            unsigned char channel,
            unsigned char sampaChip,
            unsigned char sampaChannel)
    : mIndex(index), mConnector(connector), mChannel(channel), mSampaChip(sampaChip), mSampaChannel(sampaChannel)
    {}

    const unsigned char getIndex()        const { return mIndex;       }
    const unsigned char getConnector()    const { return mConnector;   }
    const unsigned char getChannel()      const { return mChannel;     }
    const unsigned char getSampaChip()    const { return mSampaChip;   }
    const unsigned char getSampaChannel() const { return mSampaChannel;}
  private:
    unsigned char mIndex        {0};   /// FEC number in the sector
    unsigned char mConnector    {0};   /// Connector on the FEC
    unsigned char mChannel      {0};   /// Channel on the FEC
    unsigned char mSampaChip    {0};   /// SAMPA chip on the FEC
    unsigned char mSampaChannel {0};   /// Cannel on the SAMPA chip
};

}
}

#endif
