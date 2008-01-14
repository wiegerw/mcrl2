// Author(s): Wieger Wesselink
//
// Distributed under the Boost Software License, Version 1.0.
// (See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)
//
/// \file mcrl2/atermpp/aterm_traits.h
/// \brief Traits class for terms.

#ifndef MCRL2_ATERMPP_ATERM_TRAITS_H
#define MCRL2_ATERMPP_ATERM_TRAITS_H

#include "aterm2.h"

namespace atermpp {

/// \brief Traits class for terms. It is used to specify how the term interacts
/// with the garbage collector, and how it can be converted to an ATerm.
///
template <typename T>
struct aterm_traits
{
  typedef void* aterm_type; 
  static void protect(T* t)       {}
  static void unprotect(T* t)     {}
  static void mark(T t)           {}
  static T term(const T& t)       { return t; }
  static const T* ptr(const T& t) { return &t; }
  static T* ptr(T& t)             { return &t; }
};

/// \internal
///
template <>
struct aterm_traits<ATerm>
{
  typedef ATerm aterm_type;
  static void protect(ATerm* t)       { ATprotect(t); }
  static void unprotect(ATerm* t)     { ATunprotect(t); }
  static void mark(ATerm t)           { ATmarkTerm(t); }
  static ATerm term(ATerm t)          { return t; }
  static ATerm* ptr(ATerm& t)         { return &t; }
};

/// \internal
///
template <>
struct aterm_traits<ATermList>
{
  typedef ATermList aterm_type;
  static void protect(ATermList* t)   { aterm_traits<ATerm>::protect(reinterpret_cast<ATerm*>(t)); }
  static void unprotect(ATermList* t) { aterm_traits<ATerm>::unprotect(reinterpret_cast<ATerm*>(t)); }
  static void mark(ATermList t)       { aterm_traits<ATerm>::mark(reinterpret_cast<ATerm>(t)); }
  static ATerm term(ATermList t)      { return reinterpret_cast<ATerm>(t); }
  static ATerm* ptr(ATermList& t)     { return reinterpret_cast<ATerm*>(&t); }
};

/// \internal
///
template <>
struct aterm_traits<ATermAppl>
{
  typedef ATermAppl aterm_type;
  static void protect(ATermAppl* t)   { aterm_traits<ATerm>::protect(reinterpret_cast<ATerm*>(t)); }
  static void unprotect(ATermAppl* t) { aterm_traits<ATerm>::unprotect(reinterpret_cast<ATerm*>(t)); }
  static void mark(ATermAppl t)       { aterm_traits<ATerm>::mark(reinterpret_cast<ATerm>(t)); }
  static ATerm term(ATermAppl t)      { return reinterpret_cast<ATerm>(t); }
  static ATerm* ptr(ATermAppl& t)     { return reinterpret_cast<ATerm*>(&t); }
};                                    
                                      
/// \internal
///
template <>                           
struct aterm_traits<ATermBlob>        
{                                     
  typedef ATermBlob aterm_type;       
  static void protect(ATermBlob* t)   { aterm_traits<ATerm>::protect(reinterpret_cast<ATerm*>(t)); }
  static void unprotect(ATermBlob* t) { aterm_traits<ATerm>::unprotect(reinterpret_cast<ATerm*>(t)); }
  static void mark(ATermBlob t)       { aterm_traits<ATerm>::mark(reinterpret_cast<ATerm>(t)); }
  static ATerm term(ATermBlob t)      { return reinterpret_cast<ATerm>(t); }
  static ATerm* ptr(ATermBlob& t)     { return reinterpret_cast<ATerm*>(&t); }
};                                    
                                      
/// \internal
///
template <>                           
struct aterm_traits<ATermReal>        
{                                     
  typedef ATermReal aterm_type;       
  static void protect(ATermReal* t)   { aterm_traits<ATerm>::protect(reinterpret_cast<ATerm*>(t)); }
  static void unprotect(ATermReal* t) { aterm_traits<ATerm>::unprotect(reinterpret_cast<ATerm*>(t)); }
  static void mark(ATermReal t)       { aterm_traits<ATerm>::mark(reinterpret_cast<ATerm>(t)); }
  static ATerm term(ATermReal t)      { return reinterpret_cast<ATerm>(t); }
  static ATerm* ptr(ATermReal& t)     { return reinterpret_cast<ATerm*>(&t); }
};                                    
                                      
/// \internal
///
template <>                           
struct aterm_traits<ATermInt>         
{                                     
  typedef ATermInt aterm_type;        
  static void protect(ATermInt* t)    { aterm_traits<ATerm>::protect(reinterpret_cast<ATerm*>(t)); }
  static void unprotect(ATermInt* t)  { aterm_traits<ATerm>::unprotect(reinterpret_cast<ATerm*>(t)); }
  static void mark(ATermInt t)        { aterm_traits<ATerm>::mark(reinterpret_cast<ATerm>(t)); }
  static ATerm term(ATermInt t)       { return reinterpret_cast<ATerm>(t); }
  static ATerm* ptr(ATermInt& t)      { return reinterpret_cast<ATerm*>(&t); }
};                                    
                                      
} // namespace atermpp

#endif // MCRL2_ATERMPP_ATERM_TRAITS_H
