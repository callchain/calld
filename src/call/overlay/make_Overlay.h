//------------------------------------------------------------------------------
/*
    This file is part of calld: https://github.com/call/calld
    Copyright (c) 2012, 2013 Ripple Labs Inc.

    Permission to use, copy, modify, and/or distribute this software for any
    purpose  with  or without fee is hereby granted, provided that the above
    copyright notice and this permission notice appear in all copies.

    THE  SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
    WITH  REGARD  TO  THIS  SOFTWARE  INCLUDING  ALL  IMPLIED  WARRANTIES  OF
    MERCHANTABILITY  AND  FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
    ANY  SPECIAL ,  DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
    WHATSOEVER  RESULTING  FROM  LOSS  OF USE, DATA OR PROFITS, WHETHER IN AN
    ACTION  OF  CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
    OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/
//==============================================================================

#ifndef CALL_OVERLAY_MAKE_OVERLAY_H_INCLUDED
#define CALL_OVERLAY_MAKE_OVERLAY_H_INCLUDED

#include <call/rpc/ServerHandler.h>
#include <call/overlay/Overlay.h>
#include <call/resource/ResourceManager.h>
#include <call/basics/Resolver.h>
#include <call/core/Stoppable.h>
#include <boost/asio/io_service.hpp>
#include <boost/asio/ssl/context.hpp>

namespace call {

Overlay::Setup
setup_Overlay (BasicConfig const& config);

/** Creates the implementation of Overlay. */
std::unique_ptr <Overlay>
make_Overlay (
    Application& app,
    Overlay::Setup const& setup,
    Stoppable& parent,
    ServerHandler& serverHandler,
    Resource::Manager& resourceManager,
    Resolver& resolver,
    boost::asio::io_service& io_service,
    BasicConfig const& config);

} // call

#endif