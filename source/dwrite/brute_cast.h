// brute_cast.h
//
// Copyright 2006 einaros
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//

template<typename Target, typename Source>
inline Target brute_cast(const Source s)
{
  // BOOST_STATIC_ASSERT(sizeof(Target) == sizeof(Source));
  union { Target t; Source s; } u;
  u.s = s;
  return u.t;
}
