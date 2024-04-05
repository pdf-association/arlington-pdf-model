/*
 * XMLQuery.java
 * Copyright 2020 PDF Association, Inc. https://www.pdfa.org
 *
 * This material is based upon work supported by the Defense Advanced
 * Research Projects Agency (DARPA) under Contract No. HR001119C0079.
 * Any opinions, findings and conclusions or recommendations expressed
 * in this material are those of the author(s) and do not necessarily
 * reflect the views of the Defense Advanced Research Projects Agency
 * (DARPA). Approved for public release.
 *
 * SPDX-License-Identifier: Apache-2.0
 * Contributors: Roman Toda, Frantisek Forgac, Normex
 */
package gcxml;

import java.io.File;
import java.io.IOException;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Map;
import java.util.Map.Entry;
import java.util.Set;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;
import org.w3c.dom.DOMException;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

/**
 * A class to perform XPath queries on the XML representations of Arlington.
 */
public class XMLQuery {
    /**
     * Input folder containing XML files. Typically "./xml" when run from
     * the main Arlington top folder.
     */
    private String inputFolder;

    /**
     *
     */
    private DocumentBuilder domBuilder = null;

    /**
     * List of all files in "inputFolder" - file extension is NOT checked.
     */
    private File[] files = null;

    /**
     * Constructor. Initialize instance variables
     */
    public XMLQuery() {
        this.inputFolder = System.getProperty("user.dir") + "/xml/";

        File folder = new File(inputFolder);
        this.files = folder.listFiles();
        // sort files by name alphabetically, remove folders
        ArrayList<File> arr_file = new ArrayList<>();
        for (File file : files) {
            if (file.isFile()) {
                arr_file.add(file);
            }
        }
        arr_file.sort((p1, p2) -> p1.compareTo(p2));
        this.files = new File[arr_file.size()];
        for(int i=0; i < arr_file.size(); i++){
            this.files[i] = arr_file.get(i);
        }

        try {
            DocumentBuilderFactory domFactory = DocumentBuilderFactory.newInstance();
            domBuilder = domFactory.newDocumentBuilder();
            domBuilder.newDocument();
        }
        catch (FactoryConfigurationError | Exception ex) {
            System.err.println(ex.toString());
        }
    }

    /**
     * Show keys that were introduced in the specified PDF version for each XML
     * file in the input folder.
     * For command line option "-sin &lt;version&gt;".
     *
     * @param pdfVersion  PDF version as a string e.g. "1.7"
     */
    public void SinceVersion(String pdfVersion){
        // loop through *.xml grammar files found in "/xml" directory
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try {
                    String inputFile = inputFolder + file.getName();
                    Document doc = domBuilder.parse(inputFile);
                    doc.getDocumentElement().normalize();

                    System.out.println("Working on " + file.getName());

                    // load all "object" nodes using xpath
                    XPath xPath =  XPathFactory.newInstance().newXPath();
                    String expression = "/PDF/OBJECT";
                    NodeList object_nodes = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);

                    for (int i = 0; i < object_nodes.getLength(); i++) {
                        int keyCount = 0;
                        Node nNode = object_nodes.item(i);
                        if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                            Element eElement = (Element) nNode;
                            String object_name = eElement.getAttribute("id"); // name of the object
                            NodeList entry_nodes = eElement.getElementsByTagName("ENTRY"); // load all "entry" nodes
                            System.out.println("Keys introduced in PDF version " + pdfVersion + " for object " + object_name + ":");
                            for (int j = 0; j < entry_nodes.getLength(); j++) {
                                Node entry = entry_nodes.item(j);
                                Element entry_elem = (Element) entry;
                                String nodeName = entry_elem.getElementsByTagName("NAME").item(0).getTextContent();
                                String nodeSinceVersion = entry_elem.getElementsByTagName("INTRODUCED").item(0).getTextContent();
                                if (pdfVersion.equals(nodeSinceVersion)) {
                                    System.out.println("\t/" + nodeName);
                                    keyCount++;
                                }
                            }
                        }
                        if (keyCount == 0) {
                            System.out.println("No keys were found in this object.");
                        }
                    } // for
                    System.out.println("======================================================================");
                }
                catch (SAXException | IOException | XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }

    /**
     * Show all keys and the version of PDF they were introduced for each XML
     * file in the input folder. Output will be VERY VERBOSE!!
     * For command line option "-sin -all".
     */
    public void SinceVersion(){
        // loop through *.xml grammar files found in "/xml" directory
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try {
                    String inputFile = inputFolder + file.getName();
                    Document doc = domBuilder.parse(inputFile);
                    doc.getDocumentElement().normalize();

                    System.out.println("Working on " + file.getName());

                    // load all "object" nodes using xpath
                    XPath xPath =  XPathFactory.newInstance().newXPath();
                    String expression = "/PDF/OBJECT";
                    NodeList object_nodes = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);

                    for (int i = 0; i < object_nodes.getLength(); i++) {
                        Node nNode = object_nodes.item(i);
                        if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                            Element eElement = (Element) nNode;
                            String object_name = eElement.getAttribute("id"); // name of the object
                            NodeList entry_nodes = eElement.getElementsByTagName("ENTRY"); // load all "entry" nodes
                            System.out.println("Keys in object " + object_name);
                            for (int j = 0; j < entry_nodes.getLength(); j++) {
                                Node entry = entry_nodes.item(j);
                                Element entry_elem = (Element) entry;
                                String nodeName = entry_elem.getElementsByTagName("NAME").item(0).getTextContent();
                                String nodeSinceVersion = entry_elem.getElementsByTagName("INTRODUCED").item(0).getTextContent();
                                System.out.println("\t/" + nodeName + " [" + nodeSinceVersion + "]");
                            }
                        }
                    }
                    System.out.println("======================================================================");
                }
                catch (SAXException | IOException | XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }


    /**
     * Show deprecated keys that match a specified PDF version for each XML
     * file in the input folder. For command line option "-dep &lt;version&gt;"
     *
     * @param pdfVersion  PDF version as a string e.g. "1.7"
     */
    public void DeprecatedIn(String pdfVersion){
        // loop through *.xml grammar files found in "/xml" directory
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try {
                    String inputFile = inputFolder + file.getName();
                    Document doc = domBuilder.parse(inputFile);
                    doc.getDocumentElement().normalize();

                    System.out.println("Working on " + file.getName());

                    // load all "object" nodes using xpath
                    XPath xPath =  XPathFactory.newInstance().newXPath();
                    String expression = "/PDF/OBJECT";
                    NodeList object_nodes = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);

                    for (int i = 0; i < object_nodes.getLength(); i++) {
                        int keyCount = 0;
                        Node nNode = object_nodes.item(i);
                        if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                            Element eElement = (Element) nNode;
                            String object_name = eElement.getAttribute("id"); // name of the object
                            NodeList entry_nodes = eElement.getElementsByTagName("ENTRY"); // load all "entry" nodes
                            System.out.println("Keys deprecated in PDF version " + pdfVersion + " for object " + object_name + ":");
                            for (int j = 0; j < entry_nodes.getLength(); j++) {
                                Node entry = entry_nodes.item(j);
                                Element entry_elem = (Element) entry;
                                String nodeName = entry_elem.getElementsByTagName("NAME").item(0).getTextContent();
                                NodeList deprecated = entry_elem.getElementsByTagName("DEPRECATED");
                                if ((deprecated != null) && (deprecated.getLength() > 0)) {
                                    String nodeDeprecated = deprecated.item(0).getTextContent();
                                    if (pdfVersion.equals(nodeDeprecated)) {
                                        System.out.println("\t/" + nodeName);
                                        keyCount++;
                                    }
                                }
                            }
                        }
                        if (keyCount == 0) {
                            System.out.println("No keys were found in this object.");
                        }
                    }
                    System.out.println("======================================================================");
                }
                catch (SAXException | IOException | XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }


    /**
     * Show keys that have been deprecated and the version of PDF they were
     * deprecated in for each XML file in the input folder. Output is VERBOSE!!
     * For command line option "-dep -all".
     */
    public void DeprecatedIn(){
        // loop through *.xml grammar files found in "/xml" directory
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try{
                    String inputFile = inputFolder + file.getName();
                    Document doc = domBuilder.parse(inputFile);
                    doc.getDocumentElement().normalize();

                    System.out.println("Working on " + file.getName());

                    // load all "object" nodes using xpath
                    XPath xPath =  XPathFactory.newInstance().newXPath();
                    String expression = "/PDF/OBJECT";
                    NodeList object_nodes = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);

                    for (int i = 0; i < object_nodes.getLength(); i++) {
                        Node nNode = object_nodes.item(i);
                        if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                            Element eElement = (Element) nNode;
                            String object_name = eElement.getAttribute("id"); // name of the object
                            NodeList entry_nodes = eElement.getElementsByTagName("ENTRY"); // load all "entry" nodes
                            System.out.println("Keys in object " + object_name);
                            for(int j = 0; j < entry_nodes.getLength(); j++){
                                Node entry = entry_nodes.item(j);
                                Element entry_elem = (Element) entry;
                                String nodeName = entry_elem.getElementsByTagName("NAME").item(0).getTextContent();
                                NodeList deprecated = entry_elem.getElementsByTagName("DEPRECATED");
                                if ((deprecated != null) && (deprecated.getLength() > 0)) {
                                    String nodeDeprecated = deprecated.item(0).getTextContent();
                                    System.out.println("\t/" + nodeName + " [" + nodeDeprecated + "]");
                                }
                            }
                        }
                    }
                    System.out.println("======================================================================");
                }
                catch (SAXException | IOException | XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }


    /**
     * A potentially schizophrenic PDF object is a PDF object which does not
     * have a Type or Subtype (or S) key to clearly identify it without context of
     * its reference from the PDF DOM. Technically multiple such objects can be
     * combined into a single PDF direct object so long as the keys are
     * mutually exclusive.
     * Command line option: "-so"
     */
    public void SchizophrenicObjects(){
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try {
                    String inputFile = inputFolder + file.getName();
                    System.out.println("XML file: " + file.getName());

                    Document doc = domBuilder.parse(inputFile);
                    doc.getDocumentElement().normalize();
                    ArrayList<String> arrListTypes;

                    String expression = "/PDF/OBJECT/ENTRY[NAME='Type']/VALUES/VALUE/../../../@id";
                    arrListTypes = evaluateXPath(doc, expression);
                    if (!arrListTypes.isEmpty()) {
                        System.out.println("\tThe following objects have /Type keys:");
                        for (String nodeValue : arrListTypes) {
                            System.out.println("\t\t" + nodeValue);
                        }
                    }

                    expression = "/PDF/OBJECT/ENTRY[NAME='Subype']/VALUES/VALUE/../../../@id";
                    arrListTypes = evaluateXPath(doc, expression);
                    if (!arrListTypes.isEmpty()) {
                        System.out.println("\tThe following objects have /Subtype keys:");
                        for (String nodeValue : arrListTypes) {
                            System.out.println("\t\t" + nodeValue);
                        }
                    }

                    expression = "/PDF/OBJECT/ENTRY[NAME='S']/VALUES/VALUE/../../../@id";
                    arrListTypes = evaluateXPath(doc, expression);
                    if (!arrListTypes.isEmpty()) {
                        System.out.println("\tThe following objects have /S (pseduonym for Subtype?) keys:");
                        for (String nodeValue : arrListTypes) {
                            System.out.println("\t\t" + nodeValue);
                        }
                    }
                    System.out.println();
                }
                catch (Exception ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }


    /**
     * Reports the occurrence count for keys. That is how often the same key
     * appears across multiple PDF objects. Command line option "-kc". Does
     * not count wildcards or integer array indices.
     */
    public void KeyOccurrenceCount() {
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try {
                    HashMap<String, Integer> keyCountMap = new HashMap<>();
                    ArrayList<String> keys = getAllKeys(file.getName());

                    if (!keys.isEmpty()){
                        for (String key : keys) {
                            if (keyCountMap.containsKey(key)) {
                                for (Entry<String, Integer> entry : keyCountMap.entrySet()) {
                                    if (entry.getKey().equals(key)) {
                                        int count = entry.getValue();
                                        ++count;
                                        keyCountMap.replace(key, count);
                                    }
                                }
                            }
                            else {
                                keyCountMap.put(key, 1);
                            }
                        }
                    }
                    printMap(file.getName(), keyCountMap);
                }
                catch (Exception ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        } // for
    }

        /**
     * Supports the "-sc" command line option to extract those keys across
     * all PDF object with "SpecialCase" definitions defined. Processes each
     * XML file in 'input_folder'.
     */
    public void getSpecialCases(){
        // loop through *.xml grammar files found in "/xml" directory
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                System.out.println("XML file: " + file.getName());
                try {
                    String inputFile = inputFolder + file.getName();
                    Document doc = domBuilder.parse(inputFile);
                    doc.getDocumentElement().normalize();

                    XPath xPath =  XPathFactory.newInstance().newXPath();

                    // Only get OBJECT nodes that have optional SPECIAL_CASE elements below
                    String expression = "/PDF/OBJECT/ENTRY/SPECIAL_CASE/../..";
                    NodeList special_case_nodes = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);

                    for (int i = 0; i < special_case_nodes.getLength(); i++) {
                        Element special_case_elem = (Element) special_case_nodes.item(i);
                        String spec_case = special_case_elem.getElementsByTagName("SPECIAL_CASE").item(0).getTextContent();
                        String key_name = special_case_elem.getElementsByTagName("NAME").item(0).getTextContent();
                        String object_id = special_case_elem.getAttribute("id");
                        System.out.printf("\tObject %-30s key /%-20s has special case: %s\n", object_id, key_name, spec_case);
                    }
                }
                catch (SAXException | IOException | XPathExpressionException | DOMException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }


    /**
     * Outputs the key occurrence map for the specified XML file. Used by "-kc"
     * command line option.
     *
     * @param file_name the XML file
     * @param mp   the map of key names and counts
     */
    private void printMap(String file_name, Map<String, Integer> mp) {
        System.out.println("XML file: " + file_name);
        final Iterator<Entry<String,Integer>> it = mp.entrySet().iterator();
        while (it.hasNext()) {
            final Map.Entry<String,Integer> pair = (Map.Entry<String,Integer>)it.next();
            System.out.println("\tKey /" +pair.getKey() + " = " + pair.getValue());
            String key = pair.getKey().toString();
            String dicts = getDictByKey(key, file_name);
            System.out.println("\t\tFound in: " + dicts);
        }
        System.out.println("============================================");
    }


    /**
     * Creates a new ArrayList of the node values returned from an XPath
     * expression. Only works with XPath NODESET expressions.
     *
     * @param document   the XML document
     * @param xpathExpression  the XPath expression
     *
     * @return an ArrayList of nodes
     *
     * @throws Exception from XPath
     */
    private static ArrayList<String> evaluateXPath(Document document, String xpathExpression) throws Exception
    {
        XPathFactory xpathFactory = XPathFactory.newInstance();
        XPath xpath = xpathFactory.newXPath();
        XPathExpression expr = xpath.compile(xpathExpression);
        NodeList nodes = (NodeList) expr.evaluate(document, XPathConstants.NODESET);

        ArrayList<String> values = new ArrayList<>();
        for (int i = 0; i < nodes.getLength(); i++) {
            values.add(nodes.item(i).getNodeValue());
        }
        return values;
    }


    /**
     * For the specified XML file, get the list of all unique Keys. Does not
     * add wildcard keys or array indices (integers).
     *
     * @param file_name  XML file
     *
     * @return ArrayList of unique key names
     *
     * @throws Exception from XPath
     */
    private ArrayList<String> getAllKeys(String file_name) throws Exception
    {
        ArrayList<String> allKeys = new ArrayList<>();

        String inputFile = inputFolder + file_name;
        Document doc = domBuilder.parse(inputFile);
        doc.getDocumentElement().normalize();

        String expression = "/PDF/OBJECT/ENTRY/NAME/text()";
        ArrayList<String> arrListTypes = evaluateXPath(doc, expression);

        if (!arrListTypes.isEmpty()) {
            for (String keyName : arrListTypes) {
                if (!keyName.contains("*") && !keyName.matches("^[0-9]+$")) {
                    allKeys.add(keyName);
                }
            }
        }
        return allKeys;
    }


    /**
     * Determines which PDF objects contain the given key by querying a
     * specific XML file.
     *
     * @param key   the key name to locate
     * @param file_name  the XML file name
     *
     * @return a comma separated list of
     */
    private String getDictByKey(String key, String file_name) {
        String dicts = "";
        try {
            String inputFile = inputFolder + file_name;
            Document doc = domBuilder.parse(inputFile);
            doc.getDocumentElement().normalize();

            XPath xPath =  XPathFactory.newInstance().newXPath();
            String expression = "/PDF/OBJECT/ENTRY";
            NodeList nodeList = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);

            for (int i = 0; i < nodeList.getLength(); i++) {
                Node nNode = nodeList.item(i);
                if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                    Element eElement = (Element) nNode;
                    String nodeName = eElement.getElementsByTagName("NAME").item(0).getTextContent();
                    if(nodeName.equals(key)){
                        Node rootNode = nNode.getParentNode();
                        Element rootElem = (Element) rootNode;
                        String objectName =  rootElem.getAttribute("id");
                        if (dicts.isBlank())
                            dicts = objectName;
                        else
                            dicts += ", " + objectName;
                    }
                }
            } // for
        }
        catch (SAXException | IOException | XPathExpressionException | DOMException ex) {
            Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
        }
        return dicts;
    }


    /**
     * Unions 2 Lists. That is creates a new list of all unique elements from
     * both lists. Logical "OR"). See
     * https://stackoverflow.com/questions/5283047/intersection-and-union-of-arraylists-in-java.
     *
     * @param <T>    list type, typical String here
     * @param list1  List 1
     * @param list2  List 2
     * @return the union of list1 and list2
     */
    public <T> List<T> union(List<T> list1, List<T> list2) {
        Set<T> set = new HashSet<>();
        set.addAll(list1);
        set.addAll(list2);
        return new ArrayList<>(set);
    }

    /**
     * Intersects 2 Lists. That is creates a new list with only common elements.
     * Logical "AND". See
     * https://stackoverflow.com/questions/5283047/intersection-and-union-of-arraylists-in-java.
     *
     * @param <T>    list type, typical String here
     * @param list1  List 1
     * @param list2  List 2
     * @return the intersection of list1 and list2
     */
    public <T> List<T> intersection(List<T> list1, List<T> list2) {
        List<T> list = new ArrayList<>();
        for (T t : list1) {
            if(list2.contains(t)) {
                list.add(t);
            }
        }
        return list;
    }


    /**
     * Finds PDF objects that all have a given set of keys (logical AND).
     *
     * @param keys COMMA separated list of keys
     */
    public void PotentialDicts(String keys){
        String[] given_keys = keys.split(",");
        System.out.println("PDF objects with these " + given_keys.length + " key(s): " + keys);
        XPath xPath =  XPathFactory.newInstance().newXPath();

        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                System.out.println("XML file: " + file.getName());
                boolean first_key = true;
                List<String> listDicts = new ArrayList<>();
                try {
                    String inputFile = inputFolder + file.getName();
                    Document doc = domBuilder.parse(inputFile);
                    doc.getDocumentElement().normalize();

                    for (String key : given_keys){
                        String expression = "/PDF/OBJECT/ENTRY[NAME='" + key + "']/../@id";
                        NodeList nodeList = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);

                        // Convert NodeList node values to a List we can intersect and union
                        List<String> keyDicts = new ArrayList<>();
                        for (int i = 0; i < nodeList.getLength(); i++) {
                            keyDicts.add(nodeList.item(i).getNodeValue());
                        }

                        if (first_key) {
                            listDicts.addAll(keyDicts);
                            first_key = false;
                        }
                        else {
                            listDicts = intersection(listDicts, keyDicts);
                        }
                        if (listDicts.isEmpty())
                            break;
                    }

                    if (!listDicts.isEmpty()) {
                        String dicts = "";
                        for (String s : listDicts)
                            if (dicts.isBlank())
                                dicts = s;
                            else
                                dicts = dicts + ", " + s;
                        System.out.println("\tObjects: " + dicts);
                    }
                    else
                        System.out.println("\tNo PDF objects match");                }
                catch (SAXException | IOException | XPathExpressionException | DOMException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
                System.out.println("================================================");
            }
        }  // for-each file
    }

}
