/// \file DigitChip.h
/// \brief Definition of the ITS DigitChip class

#ifndef ALICEO2_ITS_DIGITCHIP
#define ALICEO2_ITS_DIGITCHIP

#include <map>
#include "Rtypes.h"

class TClonesArray;

namespace AliceO2
{
  namespace ITSMFT
  {
  class Digit;
  }
}

namespace AliceO2
{
  namespace ITS
  {

    class DigitChip
    {
    public:
      DigitChip();
      ~DigitChip();

      static void setNumberOfRows(Int_t nr) { sNumOfRows = nr; }
      void reset();

      AliceO2::ITSMFT::Digit* getDigit(UShort_t row, UShort_t col);

      AliceO2::ITSMFT::Digit* addDigit(UShort_t chipid, UShort_t row, UShort_t col, Double_t charge, Double_t timestamp);

      void fillOutputContainer(TClonesArray* outputcont);

    private:
      AliceO2::ITSMFT::Digit* findDigit(Int_t idx);
      static Int_t sNumOfRows;         ///< Number of rows in the pixel matrix
      std::map<Int_t, AliceO2::ITSMFT::Digit*> mPixels; ///< Map of fired pixels
    };
  }
}

#endif /* ALICEO2_ITS_DIGITCHIP */
