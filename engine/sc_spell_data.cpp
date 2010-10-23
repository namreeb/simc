// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

enum sdata_field_type_t {
  SD_TYPE_INT = 0,
  SD_TYPE_UNSIGNED,
  SD_TYPE_DOUBLE,
  SD_TYPE_STR
};

struct sdata_field_t {
  sdata_field_type_t type;
  std::string        name;
};

static sdata_field_t _talent_data_fields[] = {
  { SD_TYPE_STR,      "name"          },
  { SD_TYPE_UNSIGNED, "id",           },
  { SD_TYPE_UNSIGNED, "flags"         },
  { SD_TYPE_UNSIGNED, "tab"           },
  { SD_TYPE_UNSIGNED, ""              }, // Class (spell_class_expr_t)
  { SD_TYPE_UNSIGNED, ""              }, // Pet class (spell_pet_class_expr_t)
  { SD_TYPE_UNSIGNED, "dependence"    },
  { SD_TYPE_UNSIGNED, "depend_rank"   },
  { SD_TYPE_UNSIGNED, "col"           },
  { SD_TYPE_UNSIGNED, "row"           },
  { SD_TYPE_UNSIGNED, ""              }, // Talent rank spell ids, unused for now
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_UNSIGNED, ""              },
};

static sdata_field_t _spell_data_fields[] = {
  { SD_TYPE_STR,      "name"          },
  { SD_TYPE_UNSIGNED, "id",           },
  { SD_TYPE_UNSIGNED, "flags"         },
  { SD_TYPE_DOUBLE,   "speed"         },
  { SD_TYPE_UNSIGNED, ""              }, // School, requires custom thing
  { SD_TYPE_INT,      "power_type"    },
  { SD_TYPE_UNSIGNED, ""              }, // Class (spell_class_expr_t)
  { SD_TYPE_UNSIGNED, ""              }, // Race (spell_race_expr_t)
  { SD_TYPE_INT,      "scaling"       },
  { SD_TYPE_UNSIGNED, "level"         },
  { SD_TYPE_DOUBLE,   "min_range"     },
  { SD_TYPE_DOUBLE,   "max_range"     },
  { SD_TYPE_UNSIGNED, "cooldown"      },
  { SD_TYPE_UNSIGNED, "gcd"           },
  { SD_TYPE_UNSIGNED, "category"      },
  { SD_TYPE_DOUBLE,   "duration"      },
  { SD_TYPE_UNSIGNED, "cost"          },
  { SD_TYPE_UNSIGNED, ""              }, // Runes (spell_rune_expr_t)
  { SD_TYPE_UNSIGNED, "power_gain"    },
  { SD_TYPE_UNSIGNED, "max_stack"     },
  { SD_TYPE_UNSIGNED, "proc_chance"   },
  { SD_TYPE_UNSIGNED, "initial_stack" },
  { SD_TYPE_INT,      "cast_min"      },
  { SD_TYPE_INT,      "cast_max"      },
  { SD_TYPE_INT,      "cast_div"      },
  { SD_TYPE_UNSIGNED, ""              }, // Effects, 0..2, not done for now
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_UNSIGNED, ""              }, // Attributes, 0..9, not done for now
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_UNSIGNED, ""              },
  { SD_TYPE_STR,      "desc"          },
  { SD_TYPE_STR,      "tooltip"       },
};

static std::string _class_strings[] = {
  "",
  "warrior",
  "paladin",
  "hunter",
  "rogue",
  "priest",
  "death_knight",
  "shaman",
  "mage",
  "warlock",
  "",
  "druid"
};

static std::string _race_strings[] = {
  "",
  "human",
  "orc",
  "dwarf",
  "night_elf",
  "undead",
  "tauren",
  "gnome",
  "troll", 
  "goblin",
  "blood_elf",
  "draenei",
  "", "", "", "", "", "", "", "", "", "",
  "worgen",
};

static std::string _pet_class_strings[] = {
  "",
  "cunning",
  "ferocity",
  "tenacity",
};

bool pred ( char a, char b ) {
    return tolower( a ) == tolower( b );
}

static bool str_compare_ci( const std::string& l,
			                const std::string& r )
{
    if ( l.size() != r.size() || l.size() == 0 )
        return false;

    return std::equal( l.begin(), l.end(), r.begin(), pred );
}

static bool str_in_str_ci( const std::string& l,
                           const std::string& r )
{
    return std::search( l.begin(), l.end(), r.begin(), r.end(), pred ) != l.end();
}

static unsigned class_str_to_mask( const std::string& str )
{
  int cls_id = -1;
  
  for ( unsigned int i = 0; i < sizeof( _class_strings ) / sizeof( std::string ); i++ )
  {
    if ( _class_strings[ i ].empty() )
      continue;
      
    if ( ! str_compare_ci( _class_strings[ i ], str ) )
      continue;
      
    cls_id = i;
    break;
  }
  
  return 1 << ( ( cls_id < 1 ) ? 0 : cls_id - 1 );
}

static unsigned race_str_to_mask( const std::string& str )
{
  int race_id = -1;

  for ( unsigned int i = 0; i < sizeof( _race_strings ) / sizeof( std::string ); i++ )
  {
    if ( _race_strings[ i ].empty() )
      continue;

    if ( ! str_compare_ci( _race_strings[ i ], str ) )
      continue;

    race_id = i;
    break;
  }

  return 1 << ( ( race_id < 1 ) ? 0 : race_id - 1 );
}

static unsigned pet_class_str_to_mask( const std::string& str )
{
  int cls_id = -1;
  
  for ( unsigned int i = 0; i < sizeof( _pet_class_strings ) / sizeof( std::string ); i++ )
  {
    if ( _pet_class_strings[ i ].empty() )
      continue;
      
    if ( ! str_compare_ci( _pet_class_strings[ i ], str ) )
      continue;
      
    cls_id = i;
    break;
  }
  
  return 1 << ( ( cls_id < 1 ) ? 0 : cls_id - 1 );
}

static unsigned school_str_to_mask( const std::string& str )
{
  unsigned mask = 0;
  
  if ( str_in_str_ci( str, "physical" ) || str_in_str_ci( str, "strike" ) )
    mask |= 0x1;
    
  if ( str_in_str_ci( str, "holy" ) || str_in_str_ci( str, "light" ) )
    mask |= 0x2;
  
  if ( str_in_str_ci( str, "fire" ) || str_in_str_ci( str, "flame" ) )
    mask |= 0x4;
    
  if ( str_in_str_ci( str, "nature" ) || str_in_str_ci( str, "storm" ) ) 
    mask |= 0x8;
  
  if ( str_in_str_ci( str, "frost" ) )
    mask |= 0x10;
  
  if ( str_in_str_ci( str, "shadow" ) )
    mask |= 0x20;
    
  if ( str_in_str_ci( str, "arcane" ) || str_in_str_ci( str, "spell" ) )
    mask |= 0x40;
    
  // Special case: Arcane + Holy
  if ( str_compare_ci( "divine", str ) )
    mask = 0x42;

  return mask;
}

// Generic spell list based expression, holds intersection, union for list
// For these expression types, you can only use two spell lists as parameters
struct spell_list_expr_t : public spell_data_expr_t
{
  spell_list_expr_t( sim_t* sim, const std::string& name, expr_data_type_t type = DATA_SPELL ) : 
    spell_data_expr_t( sim, name, type, TOK_SPELL_LIST ) { }
  
  virtual int evaluate()
  {
    const sc_array_t<uint32_t>* cref = 0;
    // Based on the data type, see what list of spell ids we should handle, and populate the 
    // result_spell_list accordingly
    switch ( data_type )
    {
      case DATA_SPELL:
      {
        for ( unsigned int i = 0; i < sim -> sim_data.m_spells_index_size; i++ )
        {
          if ( ! sim -> sim_data.m_spells_index[ i ] )
            continue;
      
          result_spell_list.push_back( sim -> sim_data.m_spells_index[ i ] -> id );
        }
        break;
      }
      case DATA_TALENT:
      {
        for ( unsigned int i = 0; i < sim -> sim_data.m_talents_index_size; i++ )
        {
          if ( ! sim -> sim_data.m_talents_index[ i ] )
            continue;
      
          result_spell_list.push_back( sim -> sim_data.m_talents_index[ i ] -> id );
        }
        break;
      }
      case DATA_TALENT_SPELL:
      {
        for ( unsigned int i = 0; i < sim -> sim_data.m_talents_index_size; i++ )
        {
          if ( ! sim -> sim_data.m_talents_index[ i ] )
            continue;
          
          for ( int j = 0; j < 3; j++ )
          {
            if ( ! sim -> sim_data.m_talents_index[ i ] -> rank_id[ j ] )
              continue;
              
            result_spell_list.push_back( sim -> sim_data.m_talents_index[ i ] -> rank_id[ j ] );
          }
        }
        break;
      }
      case DATA_CLASS_SPELL:
        cref = &( sim -> sim_data.m_class_spells );
        break;
      case DATA_RACIAL_SPELL:
        cref = &( sim -> sim_data.m_racial_spells );
        break;
      case DATA_MASTERY_SPELL:
        cref = &( sim -> sim_data.m_mastery_spells );
        break;
      case DATA_SPECIALIZATION_SPELL:
        cref = &( sim -> sim_data.m_talent_spec_spells );
        break;
      case DATA_GLYPH_SPELL:
        cref = &( sim -> sim_data.m_glyph_spells );
        break;
      case DATA_SET_BONUS_SPELL:
        cref = &( sim -> sim_data.m_set_bonus_spells );
        break;
      default:
        return TOK_UNKNOWN;
    }
    
    if ( cref )
    {
      for ( unsigned int i = 0; i < cref -> size(); i++ )
      {
        if ( cref -> raw_ref( i ) == 0 )
          continue;

        result_spell_list.push_back( cref -> raw_ref( i ) );
      }
    }
    
    std::sort( result_spell_list.begin(), result_spell_list.end() );
    std::unique( result_spell_list.begin(), result_spell_list.end() );
    
    return TOK_SPELL_LIST; 
  }
  
  // Intersect two spell lists
  virtual std::vector<uint32_t> operator&( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;

    // Only or two spell lists together
    if ( other.result_type != TOK_SPELL_LIST )
    {
      sim -> errorf( "Unsupported right side operand '%s' (%d) for operator &",
        other.name_str.c_str(),
        other.result_type );
      return std::vector<uint32_t>();
    }

    for ( std::vector<uint32_t>::const_iterator i = other.result_spell_list.begin(); i != other.result_spell_list.end(); i++ )
    {
      if ( std::find( result_spell_list.begin(), result_spell_list.end(), *i ) != result_spell_list.end() )
        res.push_back( *i );
    }
    
    return res;
  }

  // Merge two spell lists, uniqueing entries
  virtual std::vector<uint32_t> operator|( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res( result_spell_list.size() + other.result_spell_list.size() );
    
    // Only or two spell lists together
    if ( other.result_type != TOK_SPELL_LIST )
    {
      sim -> errorf( "Unsupported right side operand '%s' (%d) for operator |",
        other.name_str.c_str(),
        other.result_type );
      return std::vector<uint32_t>();
    }

    std::merge( result_spell_list.begin(), result_spell_list.end(),
                other.result_spell_list.begin(), other.result_spell_list.end(),
                res.begin() );
    
    std::unique( res.begin(), res.end() );
    
    return res;
  }
  
  // Subtract two spell lists, other from this
  virtual std::vector<uint32_t>operator-( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;

    // Only or two spell lists together
    if ( other.result_type != TOK_SPELL_LIST )
    {
      sim -> errorf( "Unsupported right side operand '%s' (%d) for operator -",
        other.name_str.c_str(),
        other.result_type );
      return std::vector<uint32_t>();
    }

    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
      if ( std::find( other.result_spell_list.begin(), other.result_spell_list.end(), *i ) == other.result_spell_list.end() )
        res.push_back( *i );
    }
    
    return res;
  }
};

struct sd_expr_binary_t : public spell_list_expr_t
{
  int                operation;
  spell_data_expr_t* left;
  spell_data_expr_t* right;
  
  sd_expr_binary_t( sim_t* sim, const std::string& n, int o, spell_data_expr_t* l, spell_data_expr_t* r ) : 
    spell_list_expr_t( sim, n ), operation( o ), left( l ), right( r ) { }
  
  virtual int evaluate()
  {
    int  left_result =  left -> evaluate();
    int right_result = right -> evaluate();
    result_type      = TOK_UNKNOWN;
    
    if ( left_result != TOK_SPELL_LIST )
    {
      sim -> errorf( "Inconsistent input types (%s and %s) for binary operator '%s', left must always be a spell list.\n", 
        left -> name(), right -> name(), name() );
      sim -> cancel();
    }
    else
    {
      result_type = TOK_SPELL_LIST;
      // Data type follows from left side operand
      data_type   = left -> data_type;
      
      switch ( operation )
      {
        case TOK_EQ:    result_spell_list = *left == *right; break;
        case TOK_NOTEQ: result_spell_list = *left != *right; break;
        case TOK_OR:    result_spell_list = *left | *right; break;
        case TOK_AND:   result_spell_list = *left & *right; break;
        case TOK_SUB:   result_spell_list = *left - *right; break;
        case TOK_LT:    result_spell_list = *left < *right; break;
        case TOK_LTEQ:  result_spell_list = *left <= *right; break;
        case TOK_GT:    result_spell_list = *left > *right; break;
        case TOK_GTEQ:  result_spell_list = *left >= *right; break;
        case TOK_IN:    result_spell_list = left -> in( *right ); break;
        case TOK_NOTIN: result_spell_list = left -> not_in( *right ); break;
        default:
          sim -> errorf( "Unsupported spell query operator %d", operation );
          result_spell_list = std::vector<uint32_t>();
          result_type = TOK_UNKNOWN;
          break;
      }
    }
    
    return result_type;
  }
};

struct spell_data_filter_expr_t : public spell_list_expr_t
{
  int                offset;
  sdata_field_type_t field_type;
  
  spell_data_filter_expr_t( sim_t* sim, expr_data_type_t type, const std::string& f_name ) :
    spell_list_expr_t( sim, f_name, type ), offset( 0 ), field_type( SD_TYPE_INT )
  {
    sdata_field_t      * fields = 0;
    unsigned             fsize;
    if ( type == DATA_TALENT )
    {
      fields = _talent_data_fields;
      fsize  = sizeof( _talent_data_fields );
    }
    else
    {
      fields = _spell_data_fields;
      fsize  = sizeof( _spell_data_fields );
    }
    
    // Figure out our offset then
    for ( unsigned int i = 0; i < fsize / sizeof( sdata_field_t ); i++ )
    {
      if ( fields[ i ].name.empty() || ! str_compare_ci( f_name, fields[ i ].name ) )
      {
        switch ( fields[ i ].type )
        {
          case SD_TYPE_INT:
          case SD_TYPE_UNSIGNED:
            offset += sizeof( int );
            break;
          case SD_TYPE_DOUBLE:
            offset += sizeof( double );
            break;
          case SD_TYPE_STR:
            offset += sizeof( const char* );
            break;
          default:
            sim -> errorf( "Unknown field type %d for %s.", 
              fields[ i ].type, 
              fields[ i ].name.c_str() );
            break;
        }
        
        continue;
      }

      field_type = fields[ i ].type;
      break;
    }
  }
  
  virtual std::vector<uint32_t>& build_list( std::vector<uint32_t>& res, const spell_data_expr_t& other, token_type_t t ) SC_CONST
  {
    const int      *int_v;
    int             oint_v;
    const double   *double_v;
    const unsigned *unsigned_v;
    unsigned        ounsigned_v;
    char           *p_spell_data;
    std::string     string_v,
                    ostring_v;

    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
      if ( data_type == DATA_TALENT )
        p_spell_data = reinterpret_cast< char*> ( sim -> sim_data.m_talents_index[ *i ] );
      else
        p_spell_data = reinterpret_cast< char* >( sim -> sim_data.m_spells_index[ *i ] );
      
      switch ( field_type )
      {
        case SD_TYPE_INT:
        {
          int_v  = reinterpret_cast< const int* >( p_spell_data + offset );
          oint_v = ( int ) other.result_num;
          switch ( t )
          {
            case TOK_LT:     if ( *int_v < oint_v  ) res.push_back( *i ); break;
            case TOK_LTEQ:   if ( *int_v <= oint_v ) res.push_back( *i ); break;
            case TOK_GT:     if ( *int_v > oint_v  ) res.push_back( *i ); break;
            case TOK_GTEQ:   if ( *int_v >= oint_v ) res.push_back( *i ); break;
            case TOK_EQ:     if ( *int_v == oint_v ) res.push_back( *i ); break;
            case TOK_NOTEQ:  if ( *int_v != oint_v ) res.push_back( *i ); break;
            default:         continue;
          }
          break;
        }
        case SD_TYPE_UNSIGNED:
        {
          unsigned_v  = reinterpret_cast< const unsigned* >( p_spell_data + offset );
          ounsigned_v = ( unsigned ) other.result_num;
          switch ( t )
          {
            case TOK_LT:     if ( *unsigned_v < ounsigned_v ) res.push_back( *i ); break;
            case TOK_LTEQ:   if ( *unsigned_v <= ounsigned_v ) res.push_back( *i ); break;
            case TOK_GT:     if ( *unsigned_v > ounsigned_v ) res.push_back( *i ); break;
            case TOK_GTEQ:   if ( *unsigned_v >= ounsigned_v ) res.push_back( *i ); break;
            case TOK_EQ:     if ( *unsigned_v == ounsigned_v ) res.push_back( *i ); break;
            case TOK_NOTEQ:  if ( *unsigned_v != ounsigned_v ) res.push_back( *i ); break;
            default: continue;
          }
          break;
        }
        case SD_TYPE_DOUBLE:
        {
          double_v  = reinterpret_cast< const double* >( p_spell_data + offset );
          switch ( t )
          {
            case TOK_LT:     if ( *double_v < other.result_num ) res.push_back( *i ); break;
            case TOK_LTEQ:   if ( *double_v <= other.result_num ) res.push_back( *i ); break;
            case TOK_GT:     if ( *double_v > other.result_num ) res.push_back( *i ); break;
            case TOK_GTEQ:   if ( *double_v >= other.result_num ) res.push_back( *i ); break;
            case TOK_EQ:     if ( *double_v == other.result_num ) res.push_back( *i ); break;
            case TOK_NOTEQ:  if ( *double_v != other.result_num ) res.push_back( *i ); break;
            default: continue;
          }
          break;
        }
        case SD_TYPE_STR:
        {
          if ( *reinterpret_cast<const char**>( p_spell_data + offset ) )
            string_v = std::string( *reinterpret_cast<const char**>( p_spell_data + offset ) );
          else
            string_v = "";
          armory_t::format( string_v );
          ostring_v = other.result_str;

          switch ( t )
          {
            case TOK_EQ:    if ( str_compare_ci( string_v, ostring_v ) ) res.push_back( *i ); break;
            case TOK_NOTEQ: if ( ! str_compare_ci( string_v, ostring_v ) ) res.push_back( *i ); break;
            case TOK_IN:    if ( ! string_v.empty() && str_in_str_ci( string_v, ostring_v ) ) res.push_back( *i ); break;
            case TOK_NOTIN: if ( ! string_v.empty() && ! str_in_str_ci( string_v, ostring_v ) ) res.push_back( *i ); break;
            default: continue;
          }
          break;
        }
        default:
        {
          break;
        }
      }
    }
    
    return res;
  }
  
  virtual std::vector<uint32_t> operator==( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;

    if ( other.result_type != TOK_NUM && other.result_type != TOK_STR )
    {
      sim -> errorf( "Unsupported expression operator == for left=%s(%d), right=%s(%d)", 
        name_str.c_str(),
        result_type,
        other.name_str.c_str(),
        other.result_type );
      return res;
    }
    
    return build_list( res, other, TOK_EQ );
  }

  virtual std::vector<uint32_t> operator!=( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    
    if ( other.result_type != TOK_NUM && other.result_type != TOK_STR )
    {
      sim -> errorf( "Unsupported expression operator != for left=%s(%d), right=%s(%d)", 
        name_str.c_str(),
        result_type,
        other.name_str.c_str(),
        other.result_type );
      return res;
    }
    
    return build_list( res, other, TOK_NOTEQ );
  }

  virtual std::vector<uint32_t> operator<( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    
    if ( other.result_type != TOK_NUM || 
      ( field_type != SD_TYPE_INT && field_type != SD_TYPE_UNSIGNED && field_type != SD_TYPE_DOUBLE )  )
    {
      sim -> errorf( "Unsupported expression operator < for left=%s(%d), right=%s(%d) or field '%s' is not a number", 
        name_str.c_str(),
        result_type,
        other.name_str.c_str(),
        other.result_type,
        name_str.c_str() );
      return res;
    }
    
    return build_list( res, other, TOK_LT );
  }
  
  virtual std::vector<uint32_t> operator<=( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    
    if ( other.result_type != TOK_NUM || 
      ( field_type != SD_TYPE_INT && field_type != SD_TYPE_UNSIGNED && field_type != SD_TYPE_DOUBLE )  )
    {
      sim -> errorf( "Unsupported expression operator <= for left=%s(%d), right=%s(%d) or field '%s' is not a number", 
        name_str.c_str(),
        result_type,
        other.name_str.c_str(),
        other.result_type,
        name_str.c_str() );
      return res;
    }
    
    return build_list( res, other, TOK_LTEQ );
  }

  virtual std::vector<uint32_t> operator>( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    
    if ( other.result_type != TOK_NUM || 
      ( field_type != SD_TYPE_INT && field_type != SD_TYPE_UNSIGNED && field_type != SD_TYPE_DOUBLE )  )
    {
      sim -> errorf( "Unsupported expression operator > for left=%s(%d), right=%s(%d) or field '%s' is not a number", 
        name_str.c_str(),
        result_type,
        other.name_str.c_str(),
        other.result_type,
        name_str.c_str() );
      return res;
    }
    
    return build_list( res, other, TOK_GT );
  }
  
  virtual std::vector<uint32_t> operator>=( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    
    if ( other.result_type != TOK_NUM || 
      ( field_type != SD_TYPE_INT && field_type != SD_TYPE_UNSIGNED && field_type != SD_TYPE_DOUBLE )  )
    {
      sim -> errorf( "Unsupported expression operator >= for left=%s(%d), right=%s(%d) or field '%s' is not a number", 
        name_str.c_str(),
        result_type,
        other.name_str.c_str(),
        other.result_type,
        name_str.c_str() );
      return res;
    }
    
    return build_list( res, other, TOK_GTEQ );
  }
  
  virtual std::vector<uint32_t> in( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    
    if ( other.result_type != TOK_STR || field_type != SD_TYPE_STR )
    {
      sim -> errorf( "Unsupported expression operator ~ for left=%s(%d), right=%s(%d) or field '%s' is not a string", 
        name_str.c_str(),
        result_type,
        other.name_str.c_str(),
        other.result_type,
        name_str.c_str() );
      return res;
    }
    
    return build_list( res, other, TOK_IN );
  }

  virtual std::vector<uint32_t> not_in( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    
    if ( other.result_type != TOK_STR || field_type != SD_TYPE_STR )
    {
      sim -> errorf( "Unsupported expression operator !~ for left=%s(%d), right=%s(%d) or field '%s' is not a string", 
        name_str.c_str(),
        result_type,
        other.name_str.c_str(),
        other.result_type,
        name_str.c_str() );
      return res;
    }
    
    return build_list( res, other, TOK_NOTIN );
  }
};

struct spell_rune_expr_t : public spell_list_expr_t
{
  static unsigned rune_cost( const std::string& s )
  {
    const char _runes[] = { 'b', 'u', 'f' };
    int n_runes[]       = { 0, 0, 0 };
    unsigned rune_mask  = 0;
    
    for ( unsigned int i = 0; i < s.size(); i++ )
    {
      for ( unsigned int j = 0; j < 3; j++ )
      {
        if ( s[ i ] == _runes[ j ] )
          n_runes[ j ]++;
      }
    }
  
    for ( unsigned int i = 0; i < 3; i++ )
    {
      for ( int j = 0; j < std::min( 2, n_runes[ i ] ); j++ )
        rune_mask |= ( 1 << ( i * 2 + j ) );
    }
    
    return rune_mask;
  }
  
  spell_rune_expr_t( sim_t* sim, expr_data_type_t type ) : spell_list_expr_t( sim, "rune", type ) { }
  
  virtual std::vector<uint32_t> operator==( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    unsigned              r = rune_cost( other.result_str );
    
    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
      if ( ! sim -> sim_data.m_spells_index[ *i ] || sim -> sim_data.m_spells_index[ *i ] -> power_type != 5 )
        continue;
        
      if ( ( sim -> sim_data.m_spells_index[ *i ] -> rune_cost & r ) == r )
        res.push_back( *i );
    }

    return res;
  }
  
  virtual std::vector<uint32_t> operator!=( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    unsigned              r = rune_cost( other.result_str );
    
    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
      if ( ! sim -> sim_data.m_spells_index[ *i ] || sim -> sim_data.m_spells_index[ *i ] -> power_type != 5 )
        continue;
        
      if ( ( sim -> sim_data.m_spells_index[ *i ] -> rune_cost & r ) != r )
        res.push_back( *i );
    }

    return res;
  }
};

struct spell_class_expr_t : public spell_list_expr_t
{
  spell_class_expr_t( sim_t* sim, expr_data_type_t type ) : spell_list_expr_t( sim, "class", type ) { }
  
  virtual std::vector<uint32_t> operator==( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    uint32_t              class_mask;
    
    if ( other.result_type == TOK_STR )
      class_mask = class_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do class=list
    else
      return res;
    
    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
      if ( data_type == DATA_TALENT )
      {
        if ( ! sim -> sim_data.m_talents_index[ *i ] )
          continue;

        if ( sim -> sim_data.m_talents_index[ *i ] -> m_class & class_mask )
          res.push_back( *i );
      }
      else
      {
        if ( ! sim -> sim_data.m_spells_index[ *i ] )
          continue;

        if ( sim -> sim_data.m_spells_index[ *i ] -> class_mask & class_mask )
          res.push_back( *i );
      }
    }

    return res;
  }
  
  virtual std::vector<uint32_t> operator!=( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    uint32_t              class_mask;
    
    if ( other.result_type == TOK_STR )
      class_mask = class_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do class=list
    else
      return res;
    
    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
      if ( data_type == DATA_TALENT )
      {
        if ( ! sim -> sim_data.m_talents_index[ *i ] )
          continue;
        
        if ( ( sim -> sim_data.m_talents_index[ *i ] -> m_class & class_mask ) == 0 )
          res.push_back( *i );
      }
      else
      {
        if ( ! sim -> sim_data.m_spells_index[ *i ] )
          continue;
        
        if ( ( sim -> sim_data.m_spells_index[ *i ] -> class_mask & class_mask ) == 0 )
          res.push_back( *i );
      }
    }

    return res;
  }
};

struct spell_pet_class_expr_t : public spell_list_expr_t
{
  spell_pet_class_expr_t( sim_t* sim, expr_data_type_t type ) : spell_list_expr_t( sim, "pet_class", type ) { }
  
  virtual std::vector<uint32_t> operator==( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    uint32_t              class_mask;
    
    // Pet class is only stored in talent data
    if ( data_type != DATA_TALENT )
      return res;
    
    if ( other.result_type == TOK_STR )
      class_mask = pet_class_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do class=list
    else
      return res;
    
    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
        if ( ! sim -> sim_data.m_talents_index[ *i ] )
          continue;

        if ( sim -> sim_data.m_talents_index[ *i ] -> m_pet & class_mask )
          res.push_back( *i );
    }

    return res;
  }
  
  virtual std::vector<uint32_t> operator!=( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    uint32_t              class_mask;
    
    if ( other.result_type == TOK_STR )
      class_mask = pet_class_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do class=list
    else
      return res;
    
    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
      if ( ! sim -> sim_data.m_talents_index[ *i ] )
        continue;

      if ( ( sim -> sim_data.m_talents_index[ *i ] -> m_pet & class_mask ) == 0 )
        res.push_back( *i );
    }

    return res;
  }
};

struct spell_race_expr_t : public spell_list_expr_t
{
  spell_race_expr_t( sim_t* sim, expr_data_type_t type ) : spell_list_expr_t( sim, "race", type ) { }
  
  virtual std::vector<uint32_t> operator==( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    uint32_t              race_mask;
    
    // Talents are not race specific
    if ( data_type == DATA_TALENT )
      return res;
    
    if ( other.result_type == TOK_STR )
      race_mask = race_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do race=list
    else
      return res;
    
    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
      if ( ! sim -> sim_data.m_spells_index[ *i ] )
        continue;
        
      if ( sim -> sim_data.m_spells_index[ *i ] -> race_mask & race_mask )
        res.push_back( *i );
    }

    return res;
  }
  
  virtual std::vector<uint32_t> operator!=( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    uint32_t              class_mask;
    
    // Talents are not race specific
    if ( data_type == DATA_TALENT )
      return res;
    
    if ( other.result_type == TOK_STR )
      class_mask = race_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do class=list
    else
      return res;
    
    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
      if ( ! sim -> sim_data.m_spells_index[ *i ] )
        continue;
        
      if ( ( sim -> sim_data.m_spells_index[ *i ] -> class_mask & class_mask ) == 0 )
        res.push_back( *i );
    }

    return res;
  }
};

struct spell_school_expr_t : public spell_list_expr_t
{
  spell_school_expr_t( sim_t* sim, expr_data_type_t type ) : spell_list_expr_t( sim, "school", type ) { }
  
  virtual std::vector<uint32_t> operator==( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    uint32_t              school_mask;
    
    if ( other.result_type == TOK_STR )
      school_mask = school_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do class=list
    else
      return res;
    
    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
      if ( ! sim -> sim_data.m_spells_index[ *i ] )
        continue;

      if ( ( sim -> sim_data.m_spells_index[ *i ] -> school & school_mask ) == school_mask )
        res.push_back( *i );
    }

    return res;
  }
  
  virtual std::vector<uint32_t> operator!=( const spell_data_expr_t& other )
  {
    std::vector<uint32_t> res;
    uint32_t              school_mask;
    
    if ( other.result_type == TOK_STR )
      school_mask = school_str_to_mask( other.result_str );
    // Other types will not be allowed, e.g. you cannot do school=list
    else
      return res;
    
    for ( std::vector<uint32_t>::const_iterator i = result_spell_list.begin(); i != result_spell_list.end(); i++ )
    {
      if ( ! sim -> sim_data.m_spells_index[ *i ] )
        continue;
        
      if ( ( sim -> sim_data.m_spells_index[ *i ] -> school & school_mask ) == 0 )
        res.push_back( *i );
    }

    return res;
  }
};

static spell_data_expr_t* build_expression_tree( sim_t*                     sim,
                                                 std::vector<expr_token_t>& tokens )
{
  std::vector<spell_data_expr_t*> stack;
  spell_data_expr_t* res = 0;

  int num_tokens = tokens.size();
  for( int i=0; i < num_tokens; i++ )
  {
    expr_token_t& t= tokens[ i ];
    
    if ( t.type == TOK_NUM ) 
    {
      spell_data_expr_t* ex = new spell_data_expr_t( sim, t.label, atof( t.label.c_str() ) );
      stack.push_back( ex );
    }
    else if ( t.type == TOK_STR ) 
    {
      spell_data_expr_t* e = spell_data_expr_t::create_spell_expression( sim, t.label );

      if ( ! e ) 
      {
        sim -> errorf( "Unable to decode expression function '%s'\n", t.label.c_str() );
        goto exit_label;
      }
      stack.push_back( e );
    }
    else if ( expression_t::is_binary( t.type ) )
    {
      if ( stack.size() < 2 )
        goto exit_label;
      spell_data_expr_t* right = stack.back(); stack.pop_back();
      spell_data_expr_t* left  = stack.back(); stack.pop_back();
      if ( ! left || ! right ) 
        goto exit_label;
      stack.push_back( new sd_expr_binary_t( sim, t.label, t.type, left, right ) );
    }
  }

  if ( stack.size() != 1 ) 
  {
    goto exit_label;
  }

  res = stack.back();
  stack.pop_back();
exit_label:
  while ( stack.size() )
  {
    spell_data_expr_t* s = stack.back();

    stack.pop_back();
    delete s;
  }
  return res;
}

spell_data_expr_t* spell_data_expr_t::create_spell_expression( sim_t* sim, const std::string& name_str )
{
  std::vector<std::string> splits;
  std::string              v;
  int                      num_splits = util_t::string_split( splits, name_str, "." );
  expr_data_type_t         data_type = DATA_SPELL;
  
  if ( num_splits > 2 )
    return 0;
  
  // No split, access raw list or create a normal expression
  if ( num_splits == 1 )
  {
    if ( str_compare_ci( splits[ 0 ], "spell" ) )
      return new spell_list_expr_t( sim, splits[ 0 ], DATA_SPELL );
    else if ( str_compare_ci( splits[ 0 ], "talent" ) )
      return new spell_list_expr_t( sim, splits[ 0 ], DATA_TALENT );
    else if ( str_compare_ci( splits[ 0 ], "talent_spell" ) )
      return new spell_list_expr_t( sim, splits[ 0 ], DATA_TALENT_SPELL );
    else if ( str_compare_ci( splits[ 0 ], "class_spell" ) )
      return new spell_list_expr_t( sim, splits[ 0 ], DATA_CLASS_SPELL );
    else if ( str_compare_ci( splits[ 0 ], "race_spell" ) )
      return new spell_list_expr_t( sim, splits[ 0 ], DATA_RACIAL_SPELL );
    else if ( str_compare_ci( splits[ 0 ], "mastery" ) )
      return new spell_list_expr_t( sim, splits[ 0 ], DATA_MASTERY_SPELL );
    else if ( str_compare_ci( splits[ 0 ], "spec_spell" ) )
      return new spell_list_expr_t( sim, splits[ 0 ], DATA_SPECIALIZATION_SPELL );
    else if ( str_compare_ci( splits[ 0 ], "glyph" ) )
      return new spell_list_expr_t( sim, splits[ 0 ], DATA_GLYPH_SPELL );
    else if ( str_compare_ci( splits[ 0 ], "set_bonus" ) )
      return new spell_list_expr_t( sim, splits[ 0 ], DATA_SET_BONUS_SPELL );
    else
    {
      v = name_str;
      return new spell_data_expr_t( sim, name_str, v );
    }
  }
  else // Define data type
  {
    if ( str_compare_ci( splits[ 0 ], "spell" ) )
      data_type = DATA_SPELL;
    else if ( str_compare_ci( splits[ 0 ], "talent" ) )
      data_type = DATA_TALENT;
    else if ( str_compare_ci( splits[ 0 ], "talent_spell" ) )
      data_type = DATA_TALENT_SPELL;
    else if ( str_compare_ci( splits[ 0 ], "class_spell" ) )
      data_type = DATA_CLASS_SPELL;
    else if ( str_compare_ci( splits[ 0 ], "race_spell" ) )
      data_type = DATA_RACIAL_SPELL;
    else if ( str_compare_ci( splits[ 0 ], "mastery" ) )
      data_type = DATA_MASTERY_SPELL;
    else if ( str_compare_ci( splits[ 0 ], "spec_spell" ) )
      data_type = DATA_SPECIALIZATION_SPELL;
    else if ( str_compare_ci( splits[ 0 ], "glyph" ) )
      data_type = DATA_GLYPH_SPELL;
    else if ( str_compare_ci( splits[ 0 ], "set_bonus" ) )
      data_type = DATA_SET_BONUS_SPELL;
    
    if ( str_compare_ci( splits[ 1 ], "class" ) )
      return new spell_class_expr_t( sim, data_type );
    else if ( str_compare_ci( splits[ 1 ], "race" ) )
      return new spell_race_expr_t( sim, data_type );
    else if ( data_type == DATA_TALENT && str_compare_ci( splits[ 1 ], "pet_class" ) )
      return new spell_pet_class_expr_t( sim, data_type );
    else if ( data_type != DATA_TALENT && str_compare_ci( splits[ 1 ], "school" ) )
      return new spell_school_expr_t( sim, data_type );
    else if ( data_type != DATA_TALENT && str_compare_ci( splits[ 1 ], "rune" ) )
      return new spell_rune_expr_t( sim, data_type );
    else
    {
      sdata_field_t* s = 0;
      sdata_field_t* fields = 0;
      unsigned       fsize;
      if ( data_type == DATA_TALENT )
      {
        fields = _talent_data_fields;
        fsize  = sizeof( _talent_data_fields );
      }
      else
      {
        fields = _spell_data_fields;
        fsize  = sizeof( _spell_data_fields );
      }
      
      for ( unsigned int i = 0; i < fsize / sizeof( sdata_field_t ); i++ )
      {
        if ( ! fields[ i ].name.empty() && str_compare_ci( splits[ 1 ], fields[ i ].name ) )
        {
          s = &fields[ i ];
          break;
        }
      }
      
      if ( s )
        return new spell_data_filter_expr_t( sim, data_type, s -> name );
      else
        return 0;
    }
  }
  
  return 0;
}

spell_data_expr_t* spell_data_expr_t::parse( sim_t* sim, const std::string& expr_str )
{
  if ( expr_str.empty() ) return 0;

  std::vector<expr_token_t> tokens;

  expression_t::parse_tokens( 0, tokens, expr_str );

  if ( sim -> debug ) expression_t::print_tokens( tokens );

  expression_t::convert_to_unary( 0, tokens );

  if ( sim -> debug ) expression_t::print_tokens( tokens );

  if( ! expression_t::convert_to_rpn( 0, tokens ) ) 
  {
    sim -> errorf( "Unable to convert %s into RPN\n", expr_str.c_str() );
    sim -> cancel();
    return 0;
  }

  if ( sim -> debug ) expression_t::print_tokens( tokens );

  spell_data_expr_t* e = build_expression_tree( sim, tokens );

  if ( ! e ) 
  {
    sim -> errorf( "Unable to build expression tree from %s\n", expr_str.c_str() );
    sim -> cancel();
    return 0;
  }

  return e;
}
