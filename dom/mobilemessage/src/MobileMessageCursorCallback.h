/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef mozilla_dom_mobilemessage_MobileMessageCursorCallback_h
#define mozilla_dom_mobilemessage_MobileMessageCursorCallback_h

#include "mozilla/Attributes.h"
#include "mozilla/dom/DOMCursor.h"
#include "nsIMobileMessageCursorCallback.h"
#include "nsCycleCollectionParticipant.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

class nsICursorContinueCallback;

namespace mozilla {
namespace dom {

class MobileMessageManager;

namespace mobilemessage {

class MobileMessageCursorCallback;

class MobileMessageCursor MOZ_FINAL : public DOMCursor
{
  friend class MobileMessageCursorCallback;

public:
  NS_DECL_ISUPPORTS_INHERITED

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MobileMessageCursor, DOMCursor)

  MobileMessageCursor(nsPIDOMWindow* aWindow,
                      nsICursorContinueCallback *aCallback);

  virtual void
  Continue(ErrorResult& aRv);

private:
  ~MobileMessageCursor() {}

private:
  // List of read-ahead results in reversed order.
  nsTArray<nsCOMPtr<nsISupports>> mResults;
};

class MobileMessageCursorCallback MOZ_FINAL : public nsIMobileMessageCursorCallback
{
  friend class mozilla::dom::MobileMessageManager;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSIMOBILEMESSAGECURSORCALLBACK

  NS_DECL_CYCLE_COLLECTION_CLASS(MobileMessageCursorCallback)

  MobileMessageCursorCallback()
  {
    MOZ_COUNT_CTOR(MobileMessageCursorCallback);
  }

private:
  ~MobileMessageCursorCallback()
  {
    MOZ_COUNT_DTOR(MobileMessageCursorCallback);
  }

  nsRefPtr<MobileMessageCursor> mDOMCursor;
};

} // namespace mobilemessage
} // namespace dom
} // namespace mozilla

#endif // mozilla_dom_mobilemessage_MobileMessageCursorCallback_h

