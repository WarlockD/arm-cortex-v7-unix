#ifndef _CSTR_HPP_
#define _CSTR_HPP_


//https://sourceforge.net/p/constexprstr/code/HEAD/tree/




#include <iostream>
#include <ostream>

namespace hel
{
  struct int_pack {
    template<int ... N> struct pack { };
    template<class pack, int N> struct append;
    template<int ... L, int R> struct append<pack<L...>, R>
    {
      typedef pack<L..., R> type;
    };
    template<int begin, int end> struct range;

    template<int end> struct range<end, end> {
      typedef pack<> type;
    };

    template<int begin, int end> struct range
    {
      typedef typename append<
                typename range<begin, end - 1>::type
                  , end -1>::type  type;
    };
  };

  template<int N> struct num { };
  struct tail_op { };
  struct append_op { };
  struct prepend_op { };

  template<typename T, int MaxSize> struct array
  {
    T data[MaxSize];
    // we use pointers to array - since using a reference does not work because of bug in gcc.4.6.1
    template<int ... I, int M, template<int...> class TT>
      constexpr
        array(const T (*s)[M], TT<I...> ) : data{ (*s)[I]...} { }

    template<int ... I, int M, template<int ...> class TT>
      constexpr
        array(const T (*s)[M], TT<I...> , char c, append_op) : data{ (*s)[I]..., c} { }

    template<int ... I1, int ... I2, int M1, int M2, template<int ...> class TT1, template<int...> class TT2>
      constexpr
        array(const T (*s1)[M1], TT1<I1...>
                    , const T (*s2)[M2], TT2<I2...>
                    , append_op) : data{ (*s1)[I1]..., (*s2)[I2]... } { }


    template<int ... I, int M, template<int ...> class TT>
      constexpr
        array(const T (*s)[M], TT<I...> , char c, prepend_op) : data{ c, (*s)[I]...} { }

    template<int ... I1, int ... I2, int M1, int M2, template<int ...> class TT1, template<int...> class TT2>
      constexpr
        array(const T (*s1)[M1], TT1<I1...>
                    , const T (*s2)[M2], TT2<I2...>
                    , prepend_op) : data{ (*s2)[I1]..., (*s1)[I2]... } { }
    constexpr T operator[](int idx)
    {
       return idx >= MaxSize || idx < 0 ? throw "Bad Index" : data[idx];
    }

  };

  template<int N> struct string
  {
    array<char, N> data;
    int size;


    constexpr
      string(const char (&null_carray)[N+1]) :
          data{&null_carray, typename int_pack::range<0,N>::type{}}
          , size{N} {}

    constexpr
      string(const string<N>& s) :
          data{&s.data.data, typename int_pack::range<0,N>::type{}}
          , size{N} {}

    constexpr
        string(const string<N+1>& s, tail_op) :
          data{&s.data.data, typename int_pack::range<1,N+1>::type{}}
          , size{N} {}


    constexpr
        string(const string<N-1>& s, char c, append_op a) :
          data{&s.data.data, typename int_pack::range<0, N - 1>::type{}, c, a}
          , size{N} {}

    template<int M1>
      constexpr
        string(const string<M1>& s1, const string<N - M1>& s2, append_op a) :
          data{&s1.data.data, typename int_pack::range<0,M1>::type{}, &s2.data.data,
            typename int_pack::range<0,N - M1>::type{}, a}
          , size{N} {}

    constexpr
        string(const string<N-1>& s, char c, prepend_op a) :
          data{&s.data.data, typename int_pack::range<0,N-1>::type{}, c, a}
          , size{N} {}

   template<int M1>
      constexpr
        string(const string<M1>& s1, const string<N - M1>& s2, prepend_op a) :
          data{&s1.data.data, typename int_pack::range<0,M1>::type{}, &s2.data.data,
            typename int_pack::range<0,N - M1>::type{}, a}
          , size{N} {}


    constexpr char operator[](int idx) { return data[idx]; }
    constexpr int length() { return size; }
  //  constexpr int length() const { return size; }
     constexpr char first() const { return data[0]; }
     constexpr char last() const { return data[size-1]; }
     constexpr int end() const { return size; }
     constexpr int begin() const { return 0; }
     constexpr auto c_str() const { return data.data; }
    // constexpr char operator[](int i) const { return data[i]; }

    friend std::ostream& operator<<(std::ostream& o, const string& s)
    {

      o.write(s.data.data, s.size);
      return o;
    }
  };


  template<> struct string<0> {
    constexpr string() = default;
    constexpr string(const char (&)[1]) { }
    constexpr int length() const { return 0; }
    constexpr char first() const { return 0; }
    constexpr char last() const { return 0; }
    constexpr int end() const { return 0; }
    constexpr int begin() const { return 0; }
    friend std::ostream& operator<<(std::ostream& o, const string<0>& s)
    {
      return o;
    }
    constexpr const char* c_str() const { return ""; }
  };


  template<int N>
  constexpr
  string<N-1> cstr(const char (&s)[N])
  {
    return string<N-1>{s};
  }

  constexpr string<0> tail(const string<0>& s) { return s; }

  template<int N>
  constexpr string<N-1> tail(const string<N>& s);
  template<>
  constexpr string<0> tail<1>(const string<1>& s) { return string<0>{}; }

  template<int N>
  constexpr string<N-1> tail(const string<N>& s)
  {
    return string<N-1>{s, tail_op{}};
  }

  template<int N>
  constexpr string<N-2> tail( const char (&ca)[N] )
  {
    return tail(cstr(ca));
  }

  template<int N>
  constexpr string<N+1> append(const string<N>& s, char c)
  {
    return string<N+1>{s, c, append_op{}};
  }

  template<int N>
  constexpr string<N> append(const string<N>& s1, const string<0>& s2)
  {
    return s1;
  }

  template<int N, int M>
  constexpr string<N+M> append(const string<N>& s1, const string<M>& s2)
  {
    return string<N+M>{s1, s2, append_op{}};
  }
  template<int N, int M>
  constexpr string<N + M - 1> append(const string<N>& s1, const char (&s2)[M])
  {
    return append(s1, cstr(s2));
  }

  template<int N, int M>
  constexpr string<N + M - 2> append(const char (&s1)[N], const char (&s2)[M])
  {
    return append(cstr(s1), cstr(s2));
  }

//----------------PREPEND
  template<int N>
  constexpr string<N> prepend(const string<N>& s1, const string<0>& s2)
  {
    return s1;
  }

  template<int N>
  constexpr string<N+1> prepend(const string<N>& s, char c)
  {
    return string<N+1>{s, c, prepend_op{}};
  }

  template<int N, int M>
  constexpr string<N+M> prepend(const string<N>& s1, const string<M>& s2)
  {
    return string<N+M>{s1, s2, prepend_op{}};
  }

  template<int N, int M>
  constexpr string<N + M - 1> prepend(const string<N>& s1, const char (&s2)[M])
  {
    return prepend(s1, cstr(s2));
  }

  template<int N, int M>
  constexpr string<N + M - 2> prepend(const char (&s1)[N], const char (&s2)[M])
  {
    return prepend(cstr(s1), cstr(s2));
  }
  //

  // ---------------------- operator==
  template<int N, int M>
  constexpr bool operator==(const string<N>& s1, const string<M>& s2)
  {
    return false;
  }

  constexpr bool operator==(const string<0>& s1, const string<0>& s2)
  {
    return true;
  }

  template<int N>
  constexpr bool operator==(const string<N>& s1, const string<N>& s2)
  {
    return s1[0] == s2[0] && tail(s1) == tail(s2);
  }
  template<int N, int M>
  constexpr bool operator==(const string<N>& s1, const char (&s2)[M])
  {
    return s1 == cstr(s2);
  }
  template<int N, int M>
  constexpr bool operator==(const char (&s1)[N], const string<M> &s2)
  {
    return cstr(s1) == s2;
  }
//------
// We need partial specializations for extracting a substring - so wrap it in a class
//
namespace _impl
{


template<int N, int StartIdx, int OnePastEndIdx> struct Extracter;

// base case when there is nothing to extract
template<int N> struct Extracter<N,0,0>
{
  static_assert(0 <= N, "OnePastEndIdx must be <= N");
  static_assert(N >= 0, "N must be positive");
  static constexpr string<0> extract(const string<N> &s)
  {
    return string<0>{};
  }
};

// base case when we are done extracting
template<int N> struct Extracter<N,0,1>
{
  static_assert(1 <= N, "EndIdx must be <= N");
  static_assert(N >= 0, "N must be positive");

  static constexpr string<1> extract(const string<N> &s)
  {
    return string<1>{s[0]};
  }
};

// base case to start extracting
template<int N, int OnePastEndIdx> struct Extracter<N,0,OnePastEndIdx>
{
  static_assert(OnePastEndIdx <= N, "EndIdx must be <= N");
  static_assert(OnePastEndIdx >= 0, "EndIdx must be >=  0");
  static_assert(N >= 0, "N must be positive");

  static constexpr string<OnePastEndIdx> extract(const string<N> &s)
  {
    // start extracting and prepending until we get to the onepastendidx
    return prepend(Extracter<N - 1, 0, OnePastEndIdx - 1>::extract(tail(s)), s[0]);
  }
};

//recursive case
template<int N, int StartIdx, int OnePastEndIdx> struct Extracter
{
  static_assert(StartIdx >= 0, "StartIdx must be >= 0");
  static_assert(OnePastEndIdx <= N, "EndIdx must be <= N");
  static_assert(OnePastEndIdx >= StartIdx, "EndIdx must be >=  StartIdx");
  static_assert(N >= 0, "N must be positive");

  static constexpr string<OnePastEndIdx - StartIdx> extract(const string<N> &s)
  {
     // do nothing until we get to the StartIdx
     return Extracter<N-1, StartIdx - 1, OnePastEndIdx - 1>::extract(tail(s));
  }
};
} // endnamespace _impl

//------------------------------EXTRACT--------------------------------------------------

template<int StartIdx, int OnePastEndIdx, int N> constexpr string<OnePastEndIdx - StartIdx>
extract(const string<N>& cs) { return _impl::Extracter<N, StartIdx, OnePastEndIdx>::extract(cs); }

template<int N> constexpr string<0>
extract(const string<N>& cs, const num<0> sidx, const num<0> opeidx)
{
  return string<0>{};
}

template<int N> constexpr string<1>
extract(const string<N>& cs, const num<0> sidx, const num<1> opeidx)
{
  return string<1>{{cs[0], '\0'}};
}

template<int OnePastEndIdx, int N> constexpr string<OnePastEndIdx>
extract(const string<N>& cs, const num<0> sidx, const num<OnePastEndIdx> opeidx)
{
  return prepend(extract(tail(cs), num<0>(), num<OnePastEndIdx - 1>()), cs[0]);
}

template<int StartIdx, int OnePastEndIdx, int N> constexpr string<OnePastEndIdx - StartIdx>
extract(const string<N>& cs, const num<StartIdx> sidx, const num<OnePastEndIdx> opeidx)
{
  return extract(tail(cs), num<StartIdx - 1>(), num<OnePastEndIdx - 1>());
}

template<int StartIdx, int OnePastEndIdx, int N> constexpr string<OnePastEndIdx - StartIdx>
extract(const char (&cs)[N], const num<StartIdx> sidx = num<StartIdx>(), const num<OnePastEndIdx> opeidx = num<OnePastEndIdx>())
{
  return extract(cstr(cs), sidx, opeidx);
}
//----------------------------ENABLE_IF----------------------------
template<bool b, class ResultT = int> struct enable_if;
template<class ResultT> struct enable_if<true, ResultT> { typedef ResultT type; };

//-----------------------BLANK_STRING------------------------------
template<int N> constexpr string<N> blank_string();
template<> constexpr string<0> blank_string<0>() { return string<0>{}; }
template<> constexpr string<1> blank_string<1>() { return cstr(" "); }

template<int N> constexpr string<N> blank_string()
{
  return append(blank_string<N-1>(), ' ');
}
//----------------------------------------ADJUST_LENGTH, EXPAND_LENGTH------
template<int NewLen, int OriginalLen>
constexpr string<NewLen> adjust_length(const string<OriginalLen>& s,
    const typename enable_if<(NewLen < OriginalLen)>::type = 0)
{  return extract<0, NewLen>(s); }

template<int NewLen, int OriginalLen>
constexpr string<NewLen> adjust_length(const string<OriginalLen>& s,
    const typename enable_if<(NewLen > OriginalLen)>::type = 0)
{  return append(s, blank_string<NewLen - OriginalLen>()); }

template<int NewLen>
constexpr string<NewLen> adjust_length(const string<NewLen>& s) { return s; }

template<int NewLen, int OriginalLen>
constexpr string<NewLen> expand_length(const string<OriginalLen>& s,
    const typename enable_if<(NewLen > OriginalLen || NewLen == OriginalLen)>::type = 0)
{  return append(s, blank_string<NewLen - OriginalLen>()); }

//----------------------------- SUBSTRING -----------------
//--------------------------------------------------------------------------

template<int N> struct substring
{
  int begin_;
  int end_;
  //int length_;
  string<N> data_;

  // create a substring out of a string
  template<int M>
  constexpr substring(const string<M>& m, int begin, int end)
    : data_{expand_length<N>(m)} // ensure N is larger than or equal to M
      , begin_{begin < M ? begin : throw "Bad Index"} // begin has to be < than the size of the string
      , end_{end > begin && end <= M ? end : throw "Bad Index"}
    {  }

  // creating a substring out of a substring
  template<int M>
  constexpr substring(const substring<M>& ss, int begin, int end)
    : data_{expand_length<N>(ss.data_)}
      , begin_{ss.begin_ + begin < M ? ss.begin_ + begin : throw "Bad Index"}
      , end_{((ss.begin_ + end) > (ss.begin_ + begin) && (ss.begin_ + end) <= M)
                ? ss.begin_ + end
                : throw "Bad Index"
            }
    {  }

  constexpr int length() { return end_ - begin_; }
  constexpr char operator[](int idx)
  {
    return idx < length() ? data_[begin_ + idx] : throw "Bad Index";
  }
  friend std::ostream& operator<<(std::ostream& o, const substring& s)
  {
    o.write(s.data_.data + s.begin_, s.end_ - s.begin_);
    return o;
  }
};

//---------------------EQUAL--------------------------
template<int SubStringLen, int StringLen>
constexpr bool equal_helper(const substring<SubStringLen>& substr, const string<StringLen>& str, int cur_index = 0)
{
  return  substr.length() != str.length()
          ? false
          : (cur_index >= substr.length()
              ? true
              : substr[cur_index] == str[cur_index] && equal_helper(substr, str, cur_index + 1)
            )
  ;
}

template<int SubStringSizeL, int SubStringSizeR>
constexpr bool equal_helper(const substring<SubStringSizeL>& l, const substring<SubStringSizeR>& r, int cur_index = 0)
{
  return  l.length() != r.length()
          ? false
          : (cur_index >= l.length()
              ? true
              : l[cur_index] == r[cur_index] && equal_helper(l, r, cur_index + 1)
            )
  ;
}

template<int SubStringLen, int StringLen>
constexpr bool equal(const substring<SubStringLen>& substr, const string<StringLen>& str)
{
  return equal_helper(substr, str, 0);
}
template<int LSize, int RSize>
constexpr bool equal(const substring<LSize>& l, const substring<RSize>& r)
{
  return equal_helper(l, r, 0);
}
template<int SubStringLen, int StringLenPlusOne>
constexpr bool equal(const substring<SubStringLen>& substr, const char (&str)[StringLenPlusOne])
{
  return equal_helper(substr, cstr(str), 0);
}

template<int N, int M> constexpr bool operator==(const substring<N>& l, const substring<M>& r)
{
  return equal(l, r);
}

template<int N, int M> constexpr bool operator==(const substring<N>& l, const char (&r)[M])
{
  return equal(l, cstr(r));
}

template<int N, int M> constexpr bool operator==(const char (&l)[N], const substring<M>& r)
{
  return equal(r, cstr(l));
}


} // namespace hel














#endif
