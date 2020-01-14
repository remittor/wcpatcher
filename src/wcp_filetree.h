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

  bool is_dir()  { return (flags & EFLAG_DIRECTORY) != 0; }
  bool is_file() { return (flags & EFLAG_DIRECTORY) == 0; }
  bool is_name_case_sens() { return (flags & EFLAG_NAME_CASE_SENS) != 0; }
  bool is_content_case_sens() { return (flags & EFLAG_CONT_CASE_SENS) != 0; }
  LPCWSTR get_data_name() { return data ? (data->name ? data->name + name_pos : NULL) : NULL; }
  TElemList * get_elem_list(bool dirlist) { return is_dir() ? content.node.get_list(dirlist) : NULL; }
  void push_subelem(PTreeElem elem) noexcept;
  int set_data(PFileItem file_item) noexcept;
  int set_name(LPCWSTR elem_name, size_t elem_name_len) noexcept;
};

#pragma pack(pop)

// ==================================================================================

__forceinline
void TTreeElem::push_subelem(PTreeElem elem) noexcept
{
  if (this->is_dir()) {
    TElemList * elist = get_elem_list(elem->is_dir());
    if (!elist->head)
      elist->head = elem;    /* first elem */

    if (elist->tail)
      elist->tail->next = elem;

    elist->tail = elem;
  }
}

__forceinline
int TTreeElem::set_data(PFileItem file_item) noexcept
{
  name_pos = 0;
  data = file_item;
  if (file_item && file_item->name) {
    //bool item_is_dir = (file_item->attr & TcFileAttr::DIRECTORY) != 0;
    LPCWSTR p = wcsrchr(file_item->name, L'\\');
    if (p) {
      name_pos = (UINT16)(((size_t)p - (size_t)file_item->name) / sizeof(WCHAR));
      name_pos++;   /* skip backslash */
    }
  }
  return 0;
}

// ==================================================================================

struct FileTreeEnum {
  TTreeElem * owner;
  TTreeElem * dir;
  TTreeElem * file;
};

class FileTree
{
public:
  FileTree() noexcept;
  ~FileTree() noexcept;

  void clear() noexcept;
  void set_case_sensitive(bool is_case_sensitive) noexcept;
  void set_internal_mm(bool enabled) { m_use_mm = enabled; }
  int add_file_item(PFileItem fitem) noexcept;
  TTreeElem * find_directory(LPCWSTR curdir) noexcept;
  bool find_directory(FileTreeEnum & ftenum, LPCWSTR curdir) noexcept;

  int get_path(TTreeElem * elem, LPWSTR path, size_t path_cap, WCHAR delimiter = L'\\') noexcept;

  int get_dir_num_item(const FileTreeEnum & ftenum) noexcept;
  TTreeElem * get_next(FileTreeEnum & ftenum) noexcept;
  size_t get_num_elem() { return m_elem_count; }

private:
  void reset_root_elem() noexcept;
  void destroy_elem(PTreeElem elem, bool total_destroy = false) noexcept;
  void destroy_content(PTreeElem elem, bool total_destroy = false) noexcept;
  void unlink_elem(PTreeElem elem) noexcept;

  TTreeElem * find_subelem(PTreeElem base, LPCWSTR name, size_t name_len, bool is_dir) noexcept;
  TTreeElem * find_subdir(PTreeElem base, LPCWSTR name, size_t name_len) noexcept;
  TTreeElem * find_file(PTreeElem base, LPCWSTR name, size_t name_len) noexcept;

  TTreeElem * create_elem(PTreeElem owner, LPCWSTR name, size_t name_len, UINT16 flags) noexcept;
  TTreeElem * add_dir(PTreeElem owner, LPCWSTR name, size_t name_len) noexcept;
  TTreeElem * add_file(PTreeElem owner, LPCWSTR name, size_t name_len, PFileItem fi) noexcept;

  TTreeElem * get_last_elem(PTreeElem base, bool dirlist) { return base->is_dir() ? base->content.node.get_list(dirlist)->tail : NULL; }
  LPCWSTR get_owner_data_name(PTreeElem elem) { return elem->owner ? (elem->owner->data ? elem->owner->data->name : NULL) : NULL; }

  bool        m_case_sensitive;
  TTreeElem   m_root;
  size_t      m_elem_count;
  bool        m_use_mm;    /* use simple memory manager */
  simplemm    m_mm;
};


} /* namespace */
