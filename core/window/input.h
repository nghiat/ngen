//----------------------------------------------------------------------------//
// This file is distributed under the MIT License.                            //
// See LICENSE.txt for details.                                               //
// Copyright (C) Tran Tuan Nghia <trantuannghia95@gmail.com> 2021             //
//----------------------------------------------------------------------------//

#ifndef CORE_WINDOW_INPUT_H
#define CORE_WINDOW_INPUT_H

enum EKey {
  EKEY_NONE = 0,
  EKEY_A,
  EKEY_D,
  EKEY_S,
  EKEY_W,
  EKEY_BELOW_ESC,

  EKEY_COUNT,
};

enum EMouse {
  EMOUSE_NONE = 0,
  EMOUSE_LEFT,
  EMOUSE_MIDDLE,
  EMOUSE_RIGHT,

  EMOUSE_COUNT,
};

#endif // CORE_WINDOW_INPUT_H
