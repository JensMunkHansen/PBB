#pragma once

namespace PBB::Thread::Tags
{

//! DefaultPool
/*!
  Tag used for default pool
 */
struct DefaultPool
{
};

//! CustomPool
/*!
  Tag used for custom pool with specialized submit and worker loop
 */
struct CustomPool
{
};
}
