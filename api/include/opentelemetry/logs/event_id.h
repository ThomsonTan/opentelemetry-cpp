// Copyright The OpenTelemetry Authors
// SPDX-License-Identifier: Apache-2.0

#pragma once
#ifdef ENABLE_LOGS_PREVIEW

#  include "opentelemetry/version.h"
#  include "opentelemetry/nostd/unique_ptr.h"

OPENTELEMETRY_BEGIN_NAMESPACE
namespace logs
{

/**
 * EventId class which acts the Id of the event with an optional name.
 */
class EventId
{
public:
  EventId(int64_t id, nostd::string_view name) noexcept
  {
      id_ = id;
      name_ = std::string{name};
  }

public:
  int64_t id_;
  // nostd::unique_ptr<const char []> name_;
  std::string name_;
};

}  // namespace logs
OPENTELEMETRY_END_NAMESPACE
#endif
