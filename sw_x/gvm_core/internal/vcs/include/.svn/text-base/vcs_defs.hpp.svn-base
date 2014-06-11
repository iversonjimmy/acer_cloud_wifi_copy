//
//  Copyright 2013 Acer Cloud Technology Inc.
//  All Rights Reserved.
//
//  THIS SOFTWARE CONTAINS CONFIDENTIAL INFORMATION AND
//  TRADE SECRETS OF ACER CLOUD TECHNOLOGY INC.
//  USE, DISCLOSURE OR REPRODUCTION IS PROHIBITED WITHOUT
//  THE PRIOR EXPRESS WRITTEN PERMISSION OF ACER CLOUD
//  TECHNOLOGY INC.
//

#ifndef __VCS_DEFS_HPP_7_24_2013__
#define __VCS_DEFS_HPP_7_24_2013__

#include <vplu_types.h>

struct VcsCategory;
extern const VcsCategory VCS_CATEGORY_METADATA;
extern const VcsCategory VCS_CATEGORY_NOTES;
extern const VcsCategory VCS_CATEGORY_ASD;
extern const VcsCategory VCS_CATEGORY_SHARED_WITH_ME;
extern const VcsCategory VCS_CATEGORY_SHARED_BY_ME;

struct VcsDataset {

    /// 64-bit datasetId (from VSDS).
    u64 id;

    /// VCS-defined category string.
    /// Current valid choices are #VCS_CATEGORY_METADATA and #VCS_CATEGORY_NOTES.
    const VcsCategory& category;

    VcsDataset(u64 id, const VcsCategory& category) : id(id), category(category) {}
};

#endif // include guard
