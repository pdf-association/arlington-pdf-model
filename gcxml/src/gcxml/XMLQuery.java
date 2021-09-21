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
import java.util.Iterator;
import java.util.Map;
import java.util.Map.Entry;
import java.util.logging.Level;
import java.util.logging.Logger;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.FactoryConfigurationError;
import javax.xml.parsers.ParserConfigurationException;
import javax.xml.xpath.XPath;
import javax.xml.xpath.XPathConstants;
import javax.xml.xpath.XPathExpression;
import javax.xml.xpath.XPathExpressionException;
import javax.xml.xpath.XPathFactory;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

/**
 *
 * @author fero
 */
public class XMLQuery {
    private String inputFolder;
    
    private DocumentBuilderFactory domFactory = null;
    private DocumentBuilder domBuilder = null;
    private Document newDoc = null;
    
    private File folder = null;
    private File[] files = null;
        
    public XMLQuery() {
        this.inputFolder = System.getProperty("user.dir") + "/xml/";
        
        try {
            domFactory = DocumentBuilderFactory.newInstance();
            domBuilder = domFactory.newDocumentBuilder();
            newDoc = domBuilder.newDocument();
        } catch (FactoryConfigurationError exp) {
            System.err.println(exp.toString());
        } catch (ParserConfigurationException exp) {
            System.err.println(exp.toString());
        } catch (Exception exp) {
            System.err.println(exp.toString());
        }
        this.folder = new File(inputFolder);
        this.files = folder.listFiles();
        // sort files by name alphabetically, remove folders
        ArrayList<File> arr_file = new ArrayList<>();
        for(File file : files){
            if(file.isFile()){
                arr_file.add(file);
            }
        }
        arr_file.sort((p1, p2) -> p1.compareTo(p2));
        this.files = new File[arr_file.size()];
        for(int i=0; i < arr_file.size(); i++){
            this.files[i] = arr_file.get(i);
        }
    }
    
    // show keys that match specified pdf version
    public void SinceVersion(String pdfVersion){
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
                        int keyCount = 0;
                        Node nNode = object_nodes.item(i);
                        if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                            Element eElement = (Element) nNode;
                            String object_name = eElement.getAttribute("id"); // name of the object
                            NodeList entry_nodes = eElement.getElementsByTagName("ENTRY"); // load all "entry" nodes
                            System.out.println("Keys introduced in pdf version " + pdfVersion + " for object " + object_name + ":");
                            for(int j = 0; j < entry_nodes.getLength(); j++){
                                Node entry = entry_nodes.item(j);
                                Element entry_elem = (Element) entry;
                                String nodeName = entry_elem.getElementsByTagName("NAME").item(0).getTextContent();
                                String nodeSinceVersion = entry_elem.getElementsByTagName("INTRODUCED").item(0).getTextContent();
                                if(pdfVersion.equals(nodeSinceVersion)){
                                    System.out.println("\t/" + nodeName);
                                    keyCount++;
                                }
                            }
                        }
                        if(keyCount == 0) {
                            System.out.println("No keys were found in this object.");
                        }
                    }
                    System.out.println("======================================================================");
                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }
    
    // show all keys and their 'since version' attribute
    public void SinceVersion(){
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
                                String nodeSinceVersion = entry_elem.getElementsByTagName("INTRODUCED").item(0).getTextContent();
                                System.out.println("\t/" + nodeName + "[" + nodeSinceVersion + "]");
                            }
                        }
                    }
                    System.out.println("======================================================================");
                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }
    
    // show deprecated keys that match specified pdf version
    public void DeprecatedIn(String pdfVersion){
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
                        int keyCount = 0;
                        Node nNode = object_nodes.item(i);
                        if (nNode.getNodeType() == Node.ELEMENT_NODE) {
                            Element eElement = (Element) nNode;
                            String object_name = eElement.getAttribute("id"); // name of the object
                            NodeList entry_nodes = eElement.getElementsByTagName("ENTRY"); // load all "entry" nodes
                            System.out.println("Keys deprecated in pdf version " + pdfVersion + " for object " + object_name + ":");
                            for(int j = 0; j < entry_nodes.getLength(); j++){
                                Node entry = entry_nodes.item(j);
                                Element entry_elem = (Element) entry;
                                String nodeName = entry_elem.getElementsByTagName("NAME").item(0).getTextContent();
                                String nodeSinceVersion = entry_elem.getElementsByTagName("DEPRECATED").item(0).getTextContent();
                                if(pdfVersion.equals(nodeSinceVersion)){
                                    System.out.println("\t/" + nodeName);
                                    keyCount++;
                                }
                            }
                        }
                        if(keyCount == 0) {
                            System.out.println("No keys were found in this object.");
                        }
                    }
                    System.out.println("======================================================================");
                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }
    
    // show all keys and their 'deprecated in' attribute
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
                                String nodeSinceVersion = entry_elem.getElementsByTagName("DEPRECATED").item(0).getTextContent();
                                if(nodeSinceVersion.isBlank()){
                                   // System.out.println("\t/" + nodeName + " is not deprecated");
                                }else{
                                    System.out.println("\t/" + nodeName + "[" + nodeSinceVersion + "]");
                                }
                            }
                        }
                    }
                    System.out.println("======================================================================");
                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }
    }
    
    public void SchizophrenicObjects(){
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                try{
                String inputFile = inputFolder + file.getName();
                Document doc = domBuilder.parse(inputFile);
                doc.getDocumentElement().normalize();
                
                XPath xPath =  XPathFactory.newInstance().newXPath();
                
                String expression = "/OBJECT/ENTRY";	        
                NodeList nodeList = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);

                System.out.println("Object: " + file.getName().substring(0, file.getName().length()-4));

                for (int i = 0; i < nodeList.getLength(); i++) {
                    Node nNode = nodeList.item(i);
                    if (nNode.getNodeType() == Node.ELEMENT_NODE) {
 
                        Element eElement = (Element) nNode;
                        String nodeName = eElement.getElementsByTagName("NAME").item(0).getTextContent();
                        String nodeRequired = eElement.getElementsByTagName("REQUIRED").item(0).getTextContent();
                        
                        if((nodeName.equals("Type"))){
                            expression = "/OBJECT/ENTRY[NAME='Type']/VALUES/VALUE/TYPE/text()";
                            ArrayList<String> arrListTypes = evaluateXPath(doc, expression);

                            if(!arrListTypes.isEmpty()){  
                                Iterator<String> listIterator = arrListTypes.iterator();
                                while(listIterator.hasNext()){
                                     String nodeValue = listIterator.next();
                                    if(nodeValue.equals("name") || (nodeValue.equals("integer"))){
                                        if(nodeRequired.equals("false")){
                                            System.out.println(" contains key Type [" +nodeValue+ "] - optional");
                                        }else{
                                            System.out.println(" contains key Type [" +nodeValue+ "] - required");
                                        }
                                    }
                                }
                            }
                        }
                        if(nodeName.equals("Subtype")){
                            expression = "/OBJECT/ENTRY[NAME='Subtype']/VALUES/VALUE/TYPE/text()";
                            ArrayList<String> arrListTypes = evaluateXPath(doc, expression);

                            if(!arrListTypes.isEmpty()){
                                Iterator<String> listIterator = arrListTypes.iterator();
                                while(listIterator.hasNext()){
                                    String nodeValue = listIterator.next();
                                    if(nodeValue.equals("name") || (nodeValue.equals("integer"))){
                                        if(nodeRequired.equals("false")){
                                            System.out.println(" contains key Subtype [" +nodeValue+ "]- optional");
                                        }else{
                                            System.out.println(" contains key Subtype [" +nodeValue+ "] -required");
                                        }
                                    }
                                }
                            }                          
                        }
                        if(nodeName.equals("S")){
                            expression = "/OBJECT/ENTRY[NAME='S']/VALUES/VALUE/TYPE/text()";
                            ArrayList<String> arrListTypes = evaluateXPath(doc, expression);

                            if(!arrListTypes.isEmpty()){
                                Iterator<String> listIterator = arrListTypes.iterator();
                                while(listIterator.hasNext()){
                                    String nodeValue = listIterator.next();
                                    if(nodeValue.equals("name") || (nodeValue.equals("integer"))){
                                        if(nodeRequired.equals("false")){
                                            System.out.println(" contains key Subtype [" +nodeValue+ "] - optional");
                                        }else{
                                            System.out.println(" contains key Subtype [" +nodeValue+ "] - required");
                                        }
                                    }
                                }
                            }                          
                        }
                    } 
                }
                System.out.println();

                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (Exception ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
            }
        }    
    }
    
    public void KeyOccurrenceCount(){
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                HashMap<String, Integer> keyCountMap = new HashMap<String, Integer>();
                System.out.println("Working on " + file.getName());
                ArrayList<String> keys = new ArrayList<>();
                keys = getAllKeys(file.getName());
                
                if(!keys.isEmpty()){
                    Iterator<String> listIterator =  keys.iterator();
                    while(listIterator.hasNext()){
                        String key = listIterator.next();
                        if(keyCountMap.containsKey(key)){
                            for (Entry<String, Integer> entry : keyCountMap.entrySet()) {
                                if (entry.getKey().equals(key)) {
                                    int count = entry.getValue();
                                    ++count;
                                    keyCountMap.replace(key, count);
                                }
                            }
                        }else{
                            keyCountMap.put(key, 1);
                        }
                    }
                }              
                printMap(file.getName(),keyCountMap);
                System.out.println("============================================");
            }
        }
    }
    
    private void printMap(String file_name, Map mp) {
        String output = "";
        Iterator it = mp.entrySet().iterator();
        while (it.hasNext()) {
            Map.Entry pair = (Map.Entry)it.next();
            output += "Key /" +pair.getKey() + " = " + pair.getValue() + "\n" ;
            //System.out.println("Key /"+pair.getKey() + " = " + pair.getValue());
            String key = pair.getKey().toString();
            String dicts = getDictByKey(key,file_name);
            output += "Found in: " + dicts + "\n";
            //System.out.println("Found in: " + dicts);
        }
        System.out.println(output);
    }
    
    private static ArrayList<String> evaluateXPath(Document document, String xpathExpression) throws Exception 
    {
        XPathFactory xpathFactory = XPathFactory.newInstance();
         
        XPath xpath = xpathFactory.newXPath();
 
        ArrayList<String> values = new ArrayList<>();
        try
        {
            XPathExpression expr = xpath.compile(xpathExpression);
             
            NodeList nodes = (NodeList) expr.evaluate(document, XPathConstants.NODESET);
             
            for (int i = 0; i < nodes.getLength(); i++) {
                values.add(nodes.item(i).getNodeValue());
            }
                 
        } catch (XPathExpressionException e) {
            e.printStackTrace();
        }
        return values;
    }

    private ArrayList<String> getAllKeys(String file_name) {
        
        ArrayList<String> allKeys = new ArrayList<>();
        
        try{
            String inputFile = inputFolder + file_name;
            Document doc = domBuilder.parse(inputFile);
            doc.getDocumentElement().normalize();
            
            XPath xPath =  XPathFactory.newInstance().newXPath();
            
            String expression = "/PDF/OBJECT/ENTRY/NAME/text()";
            ArrayList<String> arrListTypes = evaluateXPath(doc, expression);
            
            
            if(!arrListTypes.isEmpty()){
                Iterator<String> listIterator = arrListTypes.iterator();
                while(listIterator.hasNext()){
                    String keyName = listIterator.next();
                    //System.out.println(keyName);
                    allKeys.add(keyName);
                }
            }
            
        } catch (SAXException ex) {
            Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
        } catch (IOException ex) {
            Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
        } catch (XPathExpressionException ex) {
            Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
        } catch (Exception ex) {
            Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
        }
        
        return allKeys;
    }

    private String getDictByKey(String key, String file_name) {
        String dicts = "";                
                try{
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
                           dicts += objectName + ", ";
                        }
                    } 
                }

                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (Exception ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
        return dicts;
    }
    
    public void PotentialDicts(String keys){
        String[] given_keys = keys.split(",");
        
        ArrayList<String> arrListDicts = null;
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                System.out.println("\nWorking on " + file.getName());
                arrListDicts = new ArrayList<>();
                try{
                    String inputFile = inputFolder + file.getName();
                    Document doc = domBuilder.parse(inputFile);
                    doc.getDocumentElement().normalize();
                    
                    XPath xPath =  XPathFactory.newInstance().newXPath();
                    String expression = "/PDF/OBJECT";
                    NodeList nodeList = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);
                    
                    for(int i = 0; i < nodeList.getLength(); i++){
                        boolean objFits = true;
                        ArrayList<String> list_of_keys = new ArrayList<>();
                        Element object_elem = (Element) nodeList.item(i);
                        NodeList entry_nodes =  object_elem.getElementsByTagName("ENTRY");
                        for(int j = 0; j < entry_nodes.getLength(); j++){
                            Element elem = (Element) entry_nodes.item(j);
                            String name_node = elem.getElementsByTagName("NAME").item(0).getTextContent();
                            list_of_keys.add(name_node);
                        }
                        for(String key : given_keys){
                            if(!list_of_keys.contains(key)){
                                objFits = false;
                                break;
                            }
                        }
                        
                        if(objFits){
                            String dict = object_elem.getAttribute("id");
                            arrListDicts.add(dict);                    
                        }
                    }             
                
                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException | XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (Exception ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
                int results = arrListDicts.size();
                System.out.println("\nPotential objects for given keys - " +keys+ " :\nNumber of potential objects: " +results);
                Iterator<String> iterator = arrListDicts.iterator();
                while(iterator.hasNext()){
                    String dict = iterator.next();
                    System.out.println("\t" + dict);
                }
                System.out.println("================================================");
            }
        }      
    }
    
    public void getSpecialCases(){
        // loop through *.xml grammar files found in "/xml" directory
        for (File file : files) {
            if (file.isFile() && file.canRead() && file.exists()) {
                System.out.println("\nWorking on " + file.getName());
                String special_case = "";
                try{
                    String inputFile = inputFolder + file.getName();
                    Document doc = domBuilder.parse(inputFile);
                    doc.getDocumentElement().normalize();
                    
                    XPath xPath =  XPathFactory.newInstance().newXPath();
                    
                    String expression = "/PDF/OBJECT/ENTRY";
                    NodeList special_case_nodes = (NodeList) xPath.compile(expression).evaluate(doc, XPathConstants.NODESET);
                    
                    for(int i = 0; i < special_case_nodes.getLength(); i++){
                        Element special_case_elem = (Element) special_case_nodes.item(i);
                        String spec_case = special_case_elem.getElementsByTagName("SPECIAL_CASE").item(0).getTextContent();
                        String key_name = special_case_elem.getElementsByTagName("NAME").item(0).getTextContent();
                        Node object_node = special_case_elem.getParentNode();
                        Element object_elem =  (Element) object_node;
                        String object_id = object_elem.getAttribute("id");
                        if(!spec_case.isEmpty()){
                            special_case += "\nObject " +object_id+ " key /" +key_name+ ": " +spec_case;
                        }
                    }
                    
                } catch (SAXException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (IOException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (XPathExpressionException ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                } catch (Exception ex) {
                    Logger.getLogger(XMLQuery.class.getName()).log(Level.SEVERE, null, ex);
                }
                System.out.println(special_case);
            }
        }
    }
}
 