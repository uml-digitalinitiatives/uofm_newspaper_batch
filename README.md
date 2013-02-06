CONTENTS OF THIS FILE
---------------------

 * summary
 * requirements
 * installation
 * configuration
 * customization
 * troubleshooting
 * faq
 * contact
 * sponsors


SUMMARY
-------

IArchive Newspaper Ingester

Scans for unit.xml files, and attempts to create newspapers from them. Some
assumptions have been made in terms of folder structure: The tiffs are located
in a separate directory at the same level as the "years"...  So something like:
* "target" directory
    * year1/
        * month1/
            * day1/
                * unit.xml
                * pages/
                    * page-id.xml
    * year2/
      [...]
    * TIFFS/
        * year1/
            * month1/
                * day1/
                    * Archival_Images/
                        * page-id.tif


REQUIREMENTS
------------

Dependent on:
* islandora_batch
* islandora_book_batch
* islandora_large_image

INSTALLATION
------------

Install as any other Drupal module.

CONFIGURATION
-------------

There are a couple relevant parameters...  They are available from the drush
help output for the command. Primarily:
* target: Used to indicate the directory in which to search for unit.xml files
* parent: Used to indicate what collection the newspaper issue objects should
  be made a member of.


CUSTOMIZATION
-------------


TROUBLESHOOTING
---------------


F.A.Q.
------


CONTACT
-------


SPONSORS
--------

