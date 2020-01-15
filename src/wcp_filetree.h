#pragma once

#include "stdafx.h"
#include "wcp_intdata.h"
#include "wcp_simplemm.h"


namespace wcp {

/* https://docs.microsoft.com/windows/win32/api/fileapi/nf-fileapi-getvolumeinformationw */
const int max_path_component_len = 255;

const UINT16 EFLAG_DIRECTORY      = 0x0001;
const UINT16 EFLAG_FILE_ADDR      = 0x0002;     /* file elem have addr */
const UINT16 EFLAG_NAME_CASE_SENS = 0x0004;     /* elem name is case sensitive (ala UNIX) */
const UINT16 EFLAG_CONT_CASE_SENS = 0x0008;     /* dir content is case sensitive (ala UNIX) */
const UINT16 EFLAG_ROOT           = 0x0010;


#pragma pack(push, 1)

struct TTreeElem;   /* forward declaration */
typedef TTreeElem * PTreeElem;


struct TElemList {
  TTreeElem   * head;
  TTreeElem   * tail;
};
typedef TElemList * PElemList;


struct TTreeNode {
  TElemList     dir;      /* subdir list */
  TElemList     file;     /* file list */

  TElemList * get_list(bool dirlist) { return dirlist ? &dir : &file; }
};
typedef TTreeNode * PTreeNode;


struct TFileAddr {
  UINT64    fileid;
};


struct TTreeElem {
  TTreeElem     * next;         /* NULL for last element */
  TTreeElem     * owner;        /* link to owner dir */
  union {
    TTreeNode     node;         /* only for directories */
    TFileAddr     addr;         /* only for files */
  } content;
  UINT16          flags;
  UINT16          name_pos;     /* name pos in data->name */
  PFileItem       data;
  SIZE_T          name_hash;    /* hash for original name or hash for lower case name (see EFLAG_NAME_CASE_SENS) */
  UINT16          name_len;     /* length of name (number of character without zero-termination) */
  WCHAR           name[1];      /* renaming possible, but required realloc struct for longer names */

  bool is_root() { return (flags & EFLAG_ROOT) != 0; }
  bool is_dir()  { return (flags & EFLAG_DIRECTORY) != 0; }
  bool is_file() { return (flags & EFLAG_DIRECTORY) == 0; }
  bool is_name_case_sens() { return (flags & EFLAG_NAME_CASE_SENS) != 0; }
  bool is_content_case_sens() { return (flags & EFLAG_CONT_CASE_SENS) != 0; }
  LPCWSTR get_data_name() { return data ? (data->name ? data->name + name_pos : NULL) : NULL; }
  TElemList * get_elem_list(bool dirlist) { return is_dir() ? content.node.get_list(dirlist) : NULL; }
  TElemList * get_elem_list(TTreeElem * base) { return get_elem_list(base->is_dir()); }
  void push_subelem(PTreeElem elem) noexcept;
  int set_data(PFileItem file_item) noexcept;
  int set_name(LPCWSTR elem_name, size_t elem_name_len) noexcept;
  int get_dir_num_of_items() noexcept;
  int get_path(LPWSTR path, size_t path_cap, WCHAR delimiter = L'\\') noexcept;
  TTreeElem * get_prev() noexcept;
  TTreeElem * get_next() noexcept { return next; }
  bool unlink() noexcept;
};

#pragma pack(pop)

// ==================================================================================

struct TDirEnum;   /* forward declaration */
typedef TDirEnum * PDirEnum;

class TTreeEnum;  /* forward declaration */


class FileTree
{
public:
  FileTree() noexcept;
  ~FileTree() noexcept;

  void clear() noexcept;
  void set_case_sensitive(bool is_case_sensitive) noexcept;
  void set_internal_mm(bool enabled) { m_use_mm = enabled; }

  int add_file_item(LPCWSTR fullname, PFileItem fitem, OUT PTreeElem * lpElem) noexcept;
  TTreeElem * find_directory(LPCWSTR curdir, WCHAR delimiter = L'\\') noexcept;
  bool find_directory(TDirEnum & direnum, LPCWSTR curdir) noexcept;
  bool find_directory(TTreeEnum & tenum, LPCWSTR curdir, size_t max_depth = 0) noexcept;

  size_t get_num_elem() { return m_elem_count; }
  size_t get_capacity() { return m_capacity; }
  bool is_overfill() { return m_elem_count == SIZE_MAX - 1; }

private:
  void reset_root_elem() noexcept;
  void destroy_elem(PTreeElem elem, bool total_destroy = false) noexcept;
  void destroy_content(PTreeElem elem, bool total_destroy = false) noexcept;
  bool unlink_elem(PTreeElem elem) noexcept;

  TTreeElem * find_subelem(PTreeElem base, LPCWSTR name, size_t name_len, bool is_dir) noexcept;
  TTreeElem * find_subdir(PTreeElem base, LPCWSTR name, size_t name_len) noexcept;
  TTreeElem * find_file(PTreeElem base, LPCWSTR name, size_t name_len) noexcept;

  TTreeElem * create_elem(PTreeElem owner, LPCWSTR name, size_t name_len, UINT16 flags) noexcept;
  TTreeElem * add_dir(PTreeElem owner, LPCWSTR name, size_t name_len) noexcept;
  TTreeElem * add_file(PTreeElem owner, LPCWSTR name, size_t name_len, PFileItem fi) noexcept;

  TTreeElem * get_last_elem(PTreeElem base, bool dirlist) { return base->is_dir() ? base->content.node.get_list(dirlist)->tail : NULL; }
  LPCWSTR get_owner_data_name(PTreeElem elem) { return elem->owner ? (elem->owner->data ? elem->owner->data->name : NULL) : NULL; }

  TTreeElem   m_root;
  size_t      m_elem_count;
  size_t      m_capacity;
  simplemm    m_mm;

  bool        m_case_sensitive;
  bool        m_use_mm;    /* use simple memory manager */
};

// ==================================================================================

#pragma pack(push, 1)

struct TDirEnum {
  TTreeElem * owner;  /* cur owner-dir */
  TTreeElem * dir;    /* cur subdir in owner */
  TTreeElem * file;   /* cur file in owner */

  void reset(TTreeElem * _owner = NULL) noexcept
  {
    owner = _owner;
    dir = NULL;
    file = NULL;
  }

  int get_num_of_items() noexcept { return owner->get_dir_num_of_items(); }
  TTreeElem * get_next() noexcept;
};

#pragma pack(pop)


class TTreeEnum
{
public:
  static const size_t max_dir_depth = 255;

  TTreeEnum() noexcept : m_root(NULL) {  }
  ~TTreeEnum() noexcept {  }

  void reset(TTreeElem * root, size_t max_depth = 0) noexcept
  {
    m_root = root;
    m_max_depth = (max_depth == 0 || max_depth > max_dir_depth) ? max_dir_depth : max_depth - 1;
    m_cur_depth = 0;
    memset(m_path, 0, sizeof(m_path));
    m_path[0].reset(m_root);
  }
  
  TTreeElem * TTreeEnum::get_next() noexcept;

private:
  TTreeElem * m_root;
  TDirEnum    m_path[max_dir_depth + 1];
  size_t      m_max_depth;
  size_t      m_cur_depth;
};


} /* namespace */
