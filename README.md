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

Bulk Newspaper Ingester

Specifically designed for XML created by iArchives.

It scans for unit.xml files under the "target directory", and attempts to
create newspapers from them.
Some assumptions have been made in terms of folder structure.
* "target" directory
    * <one or more subdirectories>/
        * unit.xml
        * pages/
            * page-id.xml
        * pdf/
            * page-id.pdf
        * Archival_Images/_
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
* namespace: Used to indicate the namespace for the PIDs generated on ingest.


CUSTOMIZATION
-------------

This is built using the Islandora_Batch module so check out the useful
documentation on that [module's page](https://github.com/Islandora/islandora_batch#customization).

The includes/batch.process.inc holds the paper and page classes. To modify
the location of the required files or to remove objects (ie. pdf, jpg) check
the batchProcess() class function.

For example.

protected function findTiff() {
  if ($this->tiffPath === NULL) {
    // Get the URI of the page.xml.
    $page_uri = $this->objectInfo['.']['page']->uri;
    // Break into components.
    $components = explode('/', $page_uri);

    // Add in component for TIFF directory.
    // Below was only needed for the ones I had sorted.
    // array_splice($components, -5, 0, 'TIFFS');
    // Change "pages" to "Archival_Images".
    array_splice($components, -2, 1, 'Archival_Images');

    // Change the file extension.
    $xml = array_pop($components);
    $components[] = pathinfo($xml, PATHINFO_FILENAME) . '.tif';

    $this->tiffPath = implode('/', $components);
  }

  return $this->tiffPath;
}

If the $page_uri in the above example is 
/dir1/newspapers/19001002/pages/Pg1.xml
then the tiffPath is set to
/dir1/newspapers/19001002/Archival_Images/Pg1.tif

If your path is different it can be modified.


CONTACT
-------
 * [Jared Whiklo](https://github.com/whikloj)


