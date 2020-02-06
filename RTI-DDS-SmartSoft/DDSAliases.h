//===================================================================================
//
//  Copyright (C) 2019 Alex Lotz
//
//        lotz@hs-ulm.de
//
//        Servicerobotik Ulm
//        Christian Schlegel
//        Ulm University of Applied Sciences
//        Prittwitzstr. 10
//        89075 Ulm
//        Germany
//
//  This file is part of the SmartSoft Component-Developer C++ API.
//
//  Redistribution and use in source and binary forms, with or without modification,
//  are permitted provided that the following conditions are met:
//
//  1. Redistributions of source code must retain the above copyright notice,
//     this list of conditions and the following disclaimer.
//
//  2. Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation
//     and/or other materials provided with the distribution.
//
//  3. Neither the name of the copyright holder nor the names of its contributors
//     may be used to endorse or promote products derived from this software
//     without specific prior written permission.
//
//  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
//  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
//  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
//  IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
//  INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
//  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
//  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
//  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
//  OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
//  OF THE POSSIBILITY OF SUCH DAMAGE.
//
//===================================================================================

#ifndef RTIDDSSMARTSOFT_DDSALIASES_H_
#define RTIDDSSMARTSOFT_DDSALIASES_H_

#include <dds/dds.hpp>

namespace SmartDDS {

using DynamicDataSample = dds::core::xtypes::DynamicData;
using LoanedDynamicDataSample = rti::core::xtypes::LoanedDynamicData;
using DynamicStructType = dds::core::xtypes::StructType;

using DynamicDataTopic = dds::topic::Topic<dds::core::xtypes::DynamicData>;
using DynamicDataFilteredTopic = dds::topic::ContentFilteredTopic<dds::core::xtypes::DynamicData>;

using DynamicDataWriter = dds::pub::DataWriter<dds::core::xtypes::DynamicData>;
using DynamicDataWriterListener = dds::pub::NoOpDataWriterListener<dds::core::xtypes::DynamicData>;
using PublicationMatchedStatus = dds::core::status::PublicationMatchedStatus;

using DynamicDataReader = dds::sub::DataReader<dds::core::xtypes::DynamicData>;
using DynamicDataReaderListener = dds::sub::NoOpDataReaderListener<dds::core::xtypes::DynamicData>;
using LivelinessChangedStatus = dds::core::status::LivelinessChangedStatus;

// this is the default compile data definition for filtered topics
using NoCompileData = rti::topic::no_compile_data_t;

} /* namespace SmartDDS */

#endif /* RTIDDSSMARTSOFT_DDSALIASES_H_ */
