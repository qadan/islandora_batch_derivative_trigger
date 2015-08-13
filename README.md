# Islandora Generate/Regenrate Collection derivatives [![Build Status](https://travis-ci.org/qadan/islandora_generate_collection_derivatives.png?branch=7.x)](https://travis-ci.org/qadan/islandora_generate_collection_derivatives)

## Summary

The Islandora Generate/Regenrate Collection Derivatives module allows for per-DSID generation or regeneration of derivatives on objects, as they are defined by the objects' content model, as well as per-metadata-mapping regeneration of objects' DC metadata, as it is defined by either DC XSLTs associated with the content model, or by Default DC XSLTs defined in the XML Forms management page.

This can be done either on a collection's management page (fieldsets are added to the collection management form to facilitate this), or via Drush (check `drush help deriv-regen` and `drush help dcmd-regen` for details).

The list of datastreams to run the tool against is generated from `hook_islandora_derivative()`, so installing other modules that implement that hook allows one to retroactively generate derivatives on objects in the collection. A checkbox (and Drush option) is provided to toggle the derivative regeneration "force" option so that only missing datastreams are generated.

DC metadata regeneration is available using any XSLT and source DSID combination defined for that particular content model via xml_form_builder. When using Drush, these mapping strategies are required. Check `drush help md-mappings` for information on how to get valid metadata mappings for objects.

## Requirements

Islandora Generate/Regenerate Collection Derivatives is dependent on the following modules:

- Islandora
- Islandora XML Form Builder
- Islandora Basic Collection

## Installation

The module can be installed using standard Drupal module installation methods.

## Troubleshooting/Issues

- The pager currently adds an extra &content_model entry to the URL query parameters after the first page. This doesn't really hurt anything, but it would be nice to fix.

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
