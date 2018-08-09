// Copyright (C) 2002-2018 Codeplay Software Limited. All Rights Reserved.

/// @file LPGPU2_AnnotationsDataPacketParser.h
///
/// @brief Concrete implementation of the BaseDataPacketParser class.
///        Handles ANNOTATIONS (Power Prorfiling) data packets.
/// @copyright
/// Copyright (C) 2002-2018 Codeplay Software Limited. All Rights Reserved.

#ifndef LPGPU2_ANNOTATIONSDATAPACKETPARSER_H_INCLUDE
#define LPGPU2_ANNOTATIONSDATAPACKETPARSER_H_INCLUDE

// Local
#include <AMDTPowerProfilingMidTier/include/LPGPU2_BaseDataPacketParser.h>

// Forward declarations
class TargetCharacteristics;
namespace lpgpu2 {
  struct BytesParser;

  namespace db {
    class LPGPU2DatabaseAdapter;
  }
}

// clang-format off
namespace lpgpu2 {

/// @brief    Concrete packet parser class. Parses Annotations type data
///           packets.
/// @warning  None.
/// @date     08/11/2017.
/// @author   Alberto Taiuti.
class AnnotationsDataPacketParser final : public BaseDataPacketParser
{
// Methods
public:
  AnnotationsDataPacketParser(db::LPGPU2DatabaseAdapter *pDataAdapter);

// Rule of 5
// Deleted
public:
  AnnotationsDataPacketParser() = delete;
  AnnotationsDataPacketParser(const AnnotationsDataPacketParser &) = delete;
  AnnotationsDataPacketParser &operator=(
      const AnnotationsDataPacketParser &) = delete;

// Default
public:
  AnnotationsDataPacketParser(AnnotationsDataPacketParser &&) = default;
  AnnotationsDataPacketParser &operator=(AnnotationsDataPacketParser &&) = default;
  ~AnnotationsDataPacketParser() = default;

// Overridden
private:
  PPFnStatus ConsumeDataImpl(BytesParser &bp) override;
  PPFnStatus FlushDataImpl() override;

// Attributes
private:
  db::LPGPU2DatabaseAdapter *m_pDataAdapter = nullptr;

}; // class AnnotationsDataPacketParser

} // namespace lpgpu2
// clang-format on

#endif // LPGPU2_ANNOTATIONSDATAPACKETPARSER_H_INCLUDE
