# Islandora Generate/Regenrate Collection Datastreams [![Build Status](https://travis-ci.org/qadan/islandora_generate_collection_datastreams.png?branch=7.x)](https://travis-ci.org/qadan/islandora_generate_collection_datastreams)

## Introduction

The Islandora Generate/Regenrate Collection Datastreams module adds two new fieldsets to collection management pages.

The first allows you to generate or regenerate any and all available derivative datastreams on selected or all objects in a collection. The list of available derivative datastreams is generated per-content-model, and the fieldset allows for the selection of which content model to process derivative generation for. The list is generated from hook_islandora_derivative(), so installing other modules that implement this hook allows one to retroatively apply that derivative generation to objects in the collection.

The second allows you to regenerate the DC datastream for selected or all objects in a collection. The fieldset allows for the DC to be regenerated using any XSLT and source DSID combination defined for that particular content model via xml_form_builder. This includes XSLT regeneration strategies set up through default DC transforms.

## Requirements

Islandora Generate/Regenerate Collection Datastreams is dependent on the following modules:

- Islandora
- Islandora XML Form Builder
- Islandora Basic Collection

## Installation

The module can be installed using standard Drupal module installation methods.

## Troubleshooting/Issues

There are a couple known issues with the module that should be patched out eventually. Be aware that:

- The pager currently adds an extra &content_model entry to the URL query parameters after the first page. This doesn't really hurt anything, but it would be nice to fix.
- There's also a slight design flaw with hook_islandora_basic_collection_build_manage_object() where it's not currently possible to get the index of the element to add pagers to. It's possible, currently, that when using other modules that implement this hook, the index of the elements could get out of whack, and the pagers could stop working.

Having problems or solved a problem? Check out the Islandora google groups for a solution.

* [Islandora Group](https://groups.google.com/forum/?hl=en&fromgroups#!forum/islandora)
* [Islandora Dev Group](https://groups.google.com/forum/?hl=en&fromgroups#!forum/islandora-dev)

## Maintainers/Sponsors

Current maintainers:

* [Daniel Aitken](https://github.com/qadan)

## Development

If you would like to contribute to this module, please check out our helpful [Documentation for Developers](https://github.com/Islandora/islandora/wiki#wiki-documentation-for-developers) info, as well as our [Developers](http://islandora.ca/developers) section on the Islandora.ca site.

## License

[GPLv3](http://www.gnu.org/licenses/gpl-3.0.txt)
