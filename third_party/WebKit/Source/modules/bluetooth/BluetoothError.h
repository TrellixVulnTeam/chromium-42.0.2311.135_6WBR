// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BluetoothError_h
#define BluetoothError_h

#include "core/dom/DOMException.h"
#include "platform/heap/Handle.h"

namespace blink {

class ScriptPromiseResolver;
struct WebBluetoothError;

// BluetoothError is used with CallbackPromiseAdapter to receive
// WebBluetoothError responses. See CallbackPromiseAdapter class comments.
class BluetoothError {
    WTF_MAKE_NONCOPYABLE(BluetoothError);
public:
    // Interface required by CallbackPromiseAdapter:
    typedef WebBluetoothError WebType;
    static PassRefPtrWillBeRawPtr<DOMException> take(ScriptPromiseResolver*, WebBluetoothError*);
    static void dispose(blink::WebBluetoothError*);

private:
    BluetoothError() = delete;
};

} // namespace blink

#endif // BluetoothError_h
