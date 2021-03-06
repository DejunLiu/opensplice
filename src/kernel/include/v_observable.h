/*
 *                         OpenSplice DDS
 *
 *   This software and documentation are Copyright 2006 to TO_YEAR PrismTech
 *   Limited, its affiliated companies and licensors. All rights reserved.
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       http://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 *
 */
#ifndef V_OBSERVABLE_H
#define V_OBSERVABLE_H

/**
 * \class v_observable
 *
 * An observable object can be observed by observer objects. State changes of an
 * observable are notified to its observers by an event object. The event object
 * describes the reason of notification.
 */

#include "v_kernel.h"
#include "v_observer.h"
#include "v_event.h"
#include "os_if.h"

#ifdef OSPL_BUILD_CORE
#define OS_API OS_API_EXPORT
#else
#define OS_API OS_API_IMPORT
#endif
/* !!!!!!!!NOTE From here no more includes are allowed!!!!!!! */

/**
 * \brief The <code>v_observable</code> cast method.
 *
 * This method casts a kernel object to a <code>v_observable</code> object.
 * Before the cast is performed, the type of the object is checked to
 * be <code>v_observable</code> or one of its subclasses.
 */
#define v_observable(_this) (C_CAST(_this,v_observable))

/**
 * Notifies all observers of the observable.
 *
 * \param _this the reference to an observable object.
 * \param event the event describes the reason of notification and
 *              is passed to the observers.
 */
OS_API void
v_observableNotify(
    v_observable _this,
    v_event event);

/**
 * Add an observer to the observable.
 * By adding an observer to the observable, the observer is notified on
 * state changes of the observable. With this notification the given
 * userData is also sent.
 *
 * \param _this    the reference to an observable object.
 * \param observer the reference to the observer object to add.
 * \param userData this data also sent when notify the given observer.
 * \return         TRUE if the observer is added, otherwise FALSE.
 */
OS_API c_bool
v_observableAddObserver(
    v_observable _this,
    v_observer observer,
    c_voidp userData);

/**
 * Removes an observer from the observable.
 * By removing an observer from the observable, the observer is no longer
 * notified on state changes of the observable.
 *
 * \param _this    the reference to an observable object.
 * \param observer the reference to the observer object to remove.
 * \param userData the reference to the userdata belonging to the proxy of the observer object to remove.
 * \return         TRUE if the observer is removed, otherwise FALSE.
 */
OS_API c_bool
v_observableRemoveObserver (
    v_observable _this,
    v_observer observer,
    void** userData);

#undef OS_API

#endif
